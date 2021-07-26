#include "contrib/songs_in_huffman.h"

class HuffmanDecoder {
    const uint8_t *_pCompressedData;
    uint16_t _total_bits;
    const Huffman *_pHuffmanTable;

    uint8_t _loaded_bits;
    uint8_t _current_mask;
    uint16_t _bits;
public:
    HuffmanDecoder() {}

    void loadNewData(
        const uint8_t *pCompressedData,
        uint16_t total_bits,
        const Huffman *pHuffmanTable)
    {
        _pCompressedData = pCompressedData;
        _total_bits = total_bits;
        _pHuffmanTable = pHuffmanTable;
        _current_mask = 0x80;
        _bits = 1;
    }

    int decode()
    {
        const Huffman *p = _pHuffmanTable;
        uint16_t current_idx;
        if (!_total_bits)
            return 0x7FFF; // THE END
        while(1) {
            _bits <<= 1;
            if (_current_mask & pgm_read_byte_near(_pCompressedData))
                _bits |= 1;
            _current_mask >>= 1;
            _total_bits--;
            if (!_current_mask) {
                _current_mask = 0x80;
                _pCompressedData++;
            }
            p = _pHuffmanTable;
            while(1) {
                current_idx = pgm_read_word_near(p); 
                if (_bits <= current_idx)
                    break; // We either found it, or jumped over the idx
                p += 2;
            }
            if (_bits == current_idx) {
                int value = pgm_read_word_near(++p);
                if (value) {
                    _bits = 1;
                    return value;
                }
            }
        }
    }
};
