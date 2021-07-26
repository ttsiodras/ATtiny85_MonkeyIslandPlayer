#!/usr/bin/env python
"""
Encode the Monkey Island PC speaker frequency/delay data
via Huffman encoding.
"""
import glob
from collections import defaultdict

from typing import List, TextIO, Dict

from HuffmanPy.engine import (
    Symbol, BinaryString, HuffmanTable,
    encode, encode_with_table
)

def get_hex(binary_string: BinaryString) -> List[str]:
    """
    From a string like 001100110011 to an array "0x33, 0x..."
    The last bits, if necessary, are padded with 0s
    (to emit the last byte)
    """
    ret = []
    while binary_string:
        while len(binary_string) < 8:
            binary_string += '0'
        hexv = int(binary_string[:8], 2)
        ret.append("0x%02x" % hexv)
        binary_string = binary_string[8:]
        if not binary_string:
            break
    return ret


def load_data(filename: str) -> List[int]:
    """
    Parse the frequency/delay data into a list[int]
    """
    return [
        int(x) if int(x) <= 65533 else -1
        for x in open(filename)]


def get_common_huffman_table(prefix: str) -> HuffmanTable:
    """
    For each category of data (frequency/delay),
    accumulate all entries; so the perfect Huffman table
    can be calculated from all of them.
    """
    data = []
    for filename in sorted(glob.glob(prefix + "_?.data")):
        print("[-] Reading", filename)
        newdata = load_data(filename)
        if newdata[-1] > 32768:
            # Ignore the period; Just clip the data, and later
            # (see below) indicate this as a looping melody
            newdata = newdata[:-1]
        # accumulate data from all melodies
        data.extend(newdata)

    # Now build a Huffman table to optimally encode all melodies
    print("[-] Creating Huffman table for all " + prefix + " data...")
    huffman_table, _ = encode(data)
    return huffman_table


def store_huffman_table(
        fout: TextIO, prefix: str, huffman_table: HuffmanTable) -> None:
    """
    How to store the table? Simple: as a sparse array!
    We just store the (huffman index, symbol) tuples.
    """
    fout.write("const Huffman codes_%s[] PROGMEM = {\n" % prefix)
    table = defaultdict(Symbol)  # type: Dict[int, Symbol]
    for sym, binary_string in huffman_table:
        # When we decompress the compressed stream,
        # we will decode by looking up the decoded huffman index
        # as we accumulate incoming bits. But that means
        # that this...
        #     int('001101', 2)
        # ...will decode to the same index as this...
        #     int('1101', 2)
        # ...as the leading zeroes mean nothing in binary conversion.
        # To make them count, we add '1' in front:
        idx = int('1' + binary_string, 2)
        table[idx] = sym
    for key, value in sorted(table.items()):
        fout.write("    " + hex(key) + ", " + hex(value) + ",\n")
    fout.write("};\n\n")


def store_compressed(
        fout: TextIO, filename: str, huffman_table: HuffmanTable) -> None:
    """
    Store the Huffman-compressed version of the melody's data.
    """
    data = load_data(filename)
    if data[-1] > 32768:
        # It's a looping melody; remove the period (last datapoint!),
        # and indicate it as a looping melody in the generated struct
        period = 65536 - data[-1]
        data = data[:-1]
        assert period == len(data)
        loops = 9  # 9 loops should be enough for everyone :-)
    else:
        loops = 0
    # Only emit looping indicators for frequencies
    # (no need to do the same for delays - we clip them in the
    #  Makefile to have the same length as the frequencies)
    if filename.startswith('freq'):
        fout.write("const int %s_loops = %d;\n" % (
            filename[:-5], loops))

    # Compress the data with our common Huffman table...
    compressed_data = encode_with_table(huffman_table, data)
    # ...and log the number of encoded bits...
    fout.write("const int %s_bits = %d;\n" % (
        filename[:-5], len(compressed_data)))
    # ...and the encoded data themselves.
    fout.write(
        "const unsigned char %s[] PROGMEM = {\n    " % (
            filename[:-5]))
    fout.write(",\n    ".join(get_hex(compressed_data)))
    fout.write("\n};\n\n")
    input_data_size_in_bits = len(data) * 16
    print("[-] Huffman encoding for " + filename, end='')
    print(": %4.1f%%" % (
        100. * len(compressed_data) / input_data_size_in_bits))


def store_array_of_melodies(fout: TextIO) -> None:
    """
    Finally, emit the global array of melodies that gives access
    to all the data.
    """

    fout.write("""
struct {
    const unsigned char *frequencyData;
    const int frequencyDataTotalBits;
    const Huffman *huffmanFrequencyCodes;
    const unsigned char *delayData;
    const int delayDataTotalBits;
    const Huffman *huffmanDelayCodes;
    const int loops;
} const g_Melodies[] = {
""")
    for i in range(len(glob.glob("frequencies_?.data"))):
        fout.write("""
    {{ frequencies_{i}, frequencies_{i}_bits, codes_frequencies,
        delay_{i}, delay_{i}_bits, codes_delay, frequencies_{i}_loops }},
""".format(i=i))
    fout.write("};\n\n")
    fout.write("#endif\n")


def main() -> None:
    """
    Code generator that translates the *.data files (created by create_data.c)
    into Huffman-compressed data inside "songs_in_huffman.h"
    """
    fout = open("songs_in_huffman.h", "w")
    fout.write("#ifndef __HUFFMAN_DATA__\n")
    fout.write("#define __HUFFMAN_DATA__\n\n")
    fout.write("typedef int16_t Huffman;\n\n")
    for prefix in ['frequencies', 'delay']:
        huffman_table = get_common_huffman_table(prefix)
        store_huffman_table(fout, prefix, huffman_table)
        for filename in sorted(glob.glob(prefix + "_?.data")):
            store_compressed(fout, filename, huffman_table)
    store_array_of_melodies(fout)


if __name__ == "__main__":
    main()
