#ifndef LZ77_H
#define LZ77_H

#include <stddef.h>
#include <stdint.h>

/* LZ77 configuration */
#define LZ77_WINDOW_SIZE 32768
#define LZ77_MIN_MATCH 3
#define LZ77_MAX_MATCH 258

/* LZ77 token */
typedef struct {
size_t distance;
size_t length;
uint8_t next_byte;
int has_match;
int has_next_byte;
} LZ77Token;

/* Compress input data using LZ77 */
int lz77_compress(
const uint8_t *input,
size_t input_size,
LZ77Token **tokens,
size_t *token_count,
size_t max_chain_length
);

/* Decompress LZ77 tokens */
int lz77_decompress(
const LZ77Token *tokens,
size_t token_count,
uint8_t **output,
size_t *output_size
);

#endif
