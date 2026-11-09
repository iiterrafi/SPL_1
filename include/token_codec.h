#ifndef TOKEN_CODEC_H
#define TOKEN_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "lz77.h"

/*
 * Each LZ77 token is serialized into exactly 6 bytes.
 *
 * Byte 0: Flags
 * Byte 1: Distance (high byte)
 * Byte 2: Distance (low byte)
 * Byte 3: Length (high byte)
 * Byte 4: Length (low byte)
 * Byte 5: Next byte
 */

#define LZ77_TOKEN_SERIALIZED_SIZE 6

/*
 * Convert LZ77 tokens into a portable byte sequence.
 *
 * Returns:
 *  0  on success
 * -1  on failure
 */
int token_codec_serialize(
    const LZ77Token *tokens,
    size_t token_count,
    uint8_t **output,
    size_t *output_size
);

/*
 * Reconstruct LZ77 tokens from their serialized representation.
 *
 * Returns:
 *  0  on success
 * -1  on failure
 */
int token_codec_deserialize(
    const uint8_t *input,
    size_t input_size,
    LZ77Token **tokens,
    size_t *token_count
);

#endif