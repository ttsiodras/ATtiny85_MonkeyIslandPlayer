#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PROGMEM

#include "songs_in_huffman.h"

void decode_huffman(
    const char *filename,
    const uint8_t *pCompressedData,
    uint16_t total_bits,
    const Huffman *pHuffmanTable)
{
    FILE *fp = fopen(filename, "w");
    uint8_t current_mask = 0x80;
    uint16_t bits = 1;
    const Huffman *p;

    while(1) {
        bits <<= 1;
        if (current_mask & *pCompressedData)
            bits |= 1;
        current_mask >>= 1;
        total_bits--;
        if (!current_mask) {
            current_mask = 0x80;
            pCompressedData++;
        }
        p = pHuffmanTable;
        while(1) {
            if (bits <= *p)
                break; // We either found it, or jumped over the idx
            p+=2;
        }
        if (bits == *p++) {
            fprintf(fp, "%d\n", *p == -1 ? 65535 : *p);
            fflush(fp);
            bits = 1;
            if (!total_bits)
                break;
        }
        if (!total_bits) {
            puts("[x] Internal error...");
            exit(1);
        }
    }
    fclose(fp);
}

int main()
{
    for(unsigned i=0; i<sizeof(g_Melodies)/sizeof(g_Melodies[0]); i++) {
        char tmp[128];
        sprintf(tmp, "f%d.data", i);
        decode_huffman(
            tmp, 
            g_Melodies[i].frequencyData,
            g_Melodies[i].frequencyDataTotalBits,
            g_Melodies[i].huffmanFrequencyCodes);
        sprintf(tmp, "d%d.data", i);
        decode_huffman(
            tmp, 
            g_Melodies[i].delayData,
            g_Melodies[i].delayDataTotalBits,
            g_Melodies[i].huffmanDelayCodes);
    }
}
