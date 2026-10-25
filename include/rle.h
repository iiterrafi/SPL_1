#ifndef RLE_H
#define RLE_H

#include <stddef.h>
#include <stdint.h>

/* Compress data using Run-Length Encoding */
int rle_compress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size);

/* Decompress data encoded using Run-Length Encoding */
int rle_decompress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size);

#endif
