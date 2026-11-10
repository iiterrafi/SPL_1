#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stddef.h>
#include <stdint.h>

/* Metadata required for decompression and verification */
typedef struct {
    uint64_t original_size;
    uint64_t rle_size;
    uint64_t token_count;
    uint64_t serialized_size;
    uint64_t meaningful_bits;

    uint32_t crc32;

    uint16_t huffman_code_lengths[256];

} CompressorMetadata;


/* Compress data using the complete hybrid pipeline */
int compressor_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **compressed_data,
    size_t *compressed_size,
    CompressorMetadata *metadata,
    size_t max_chain_length
);


/* Decompress data and verify its integrity */
int compressor_decompress(
    const uint8_t *compressed_data,
    size_t compressed_size,
    const CompressorMetadata *metadata,
    uint8_t **output,
    size_t *output_size
);

#endif