#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "token_codec.h"

/*
 * Serialize LZ77 tokens into a portable byte sequence.
 */
int token_codec_serialize(
    const LZ77Token *tokens,
    size_t token_count,
    uint8_t **output,
    size_t *output_size)
{
    if (output == NULL || output_size == NULL)
        return -1;

    *output = NULL;
    *output_size = 0;

    if (tokens == NULL && token_count > 0)
        return -1;

    if (token_count == 0)
        return 0;

    if (token_count > SIZE_MAX / LZ77_TOKEN_SERIALIZED_SIZE)
        return -1;

    size_t serialized_size =
        token_count * LZ77_TOKEN_SERIALIZED_SIZE;

    uint8_t *result = malloc(serialized_size);

    if (result == NULL)
        return -1;

    for (size_t i = 0; i < token_count; i++)
    {
        const LZ77Token *token = &tokens[i];

        if ((token->has_match != 0 && token->has_match != 1) ||
            (token->has_next_byte != 0 && token->has_next_byte != 1))
        {
            free(result);
            return -1;
        }

        if (token->has_match)
        {
            if (token->distance == 0 ||
                token->distance > LZ77_WINDOW_SIZE ||
                token->length < LZ77_MIN_MATCH ||
                token->length > LZ77_MAX_MATCH)
            {
                free(result);
                return -1;
            }
        }
        else
        {
            if (token->distance != 0 ||
                token->length != 0 ||
                !token->has_next_byte)
            {
                free(result);
                return -1;
            }
        }

        size_t position = i * LZ77_TOKEN_SERIALIZED_SIZE;

        uint8_t flags = 0;

        if (token->has_match)
            flags |= 0x01;

        if (token->has_next_byte)
            flags |= 0x02;

        result[position] = flags;

        result[position + 1] =
            (uint8_t)(token->distance >> 8);

        result[position + 2] =
            (uint8_t)(token->distance & 0xFF);

        result[position + 3] =
            (uint8_t)(token->length >> 8);

        result[position + 4] =
            (uint8_t)(token->length & 0xFF);

        result[position + 5] =
            token->has_next_byte ? token->next_byte : 0;
    }

    *output = result;
    *output_size = serialized_size;

    return 0;
}

/*
 * Deserialize bytes into LZ77 tokens.
 */
int token_codec_deserialize(
    const uint8_t *input,
    size_t input_size,
    LZ77Token **tokens,
    size_t *token_count)
{
    if (tokens == NULL || token_count == NULL)
        return -1;

    *tokens = NULL;
    *token_count = 0;

    if (input == NULL && input_size > 0)
        return -1;

    if (input_size % LZ77_TOKEN_SERIALIZED_SIZE != 0)
        return -1;

    size_t count =
        input_size / LZ77_TOKEN_SERIALIZED_SIZE;

    if (count == 0)
        return 0;

    if (count > SIZE_MAX / sizeof(LZ77Token))
        return -1;

    LZ77Token *result = malloc(count * sizeof(LZ77Token));

    if (result == NULL)
        return -1;

    for (size_t i = 0; i < count; i++)
    {
        size_t position = i * LZ77_TOKEN_SERIALIZED_SIZE;

        uint8_t flags = input[position];

        /* Reject undefined flag bits. */
        if ((flags & 0xFC) != 0)
        {
            free(result);
            return -1;
        }

        int has_match = flags & 0x01;
        int has_next_byte = (flags >> 1) & 0x01;

        size_t distance =
            ((size_t)input[position + 1] << 8) |
            input[position + 2];

        size_t length =
            ((size_t)input[position + 3] << 8) |
            input[position + 4];

        uint8_t next_byte = input[position + 5];

        if (!has_next_byte && next_byte != 0)
        {
            free(result);
            return -1;
        }

        if (has_match)
        {
            if (distance == 0 ||
                distance > LZ77_WINDOW_SIZE ||
                length < LZ77_MIN_MATCH ||
                length > LZ77_MAX_MATCH)
            {
                free(result);
                return -1;
            }
        }
        else
        {
            if (distance != 0 ||
                length != 0 ||
                !has_next_byte)
            {
                free(result);
                return -1;
            }
        }

        result[i].distance = distance;
        result[i].length = length;
        result[i].next_byte = next_byte;
        result[i].has_match = has_match;
        result[i].has_next_byte = has_next_byte;
    }

    *tokens = result;
    *token_count = count;

    return 0;
}