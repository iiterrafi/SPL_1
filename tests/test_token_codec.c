#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lz77.h"
#include "token_codec.h"

int main(void)
{
    uint8_t input[] =
        "AAAAABBBBBBBBBCCCCCCCC"
        "ABCABCABCABCABCABC"
        "DDDDDDDDDD"
        "XYZXYZXYZXYZ";

    size_t input_size = sizeof(input) - 1;

    LZ77Token *original_tokens = NULL;
    size_t original_count = 0;

    uint8_t *serialized = NULL;
    size_t serialized_size = 0;

    LZ77Token *decoded_tokens = NULL;
    size_t decoded_count = 0;

    uint8_t *reconstructed = NULL;
    size_t reconstructed_size = 0;

    printf("Testing LZ77 Token Serialization...\n\n");

    /* Step 1: Generate LZ77 tokens */
    if (lz77_compress(input, input_size,
                      &original_tokens, &original_count, 16) != 0)
        goto error;

    printf("PASS: LZ77 token generation.\n");

    /* Step 2: Serialize tokens */
    if (token_codec_serialize(original_tokens, original_count,
                              &serialized, &serialized_size) != 0)
        goto error;

    printf("PASS: Token serialization.\n");
    printf("Original tokens: %zu\n", original_count);
    printf("Serialized size: %zu bytes\n\n", serialized_size);

    /* Step 3: Deserialize tokens */
    if (token_codec_deserialize(serialized, serialized_size,
                                &decoded_tokens, &decoded_count) != 0)
        goto error;

    printf("PASS: Token deserialization.\n");

    /* Step 4: Compare reconstructed tokens */
    if (original_count != decoded_count)
        goto error;

    for (size_t i = 0; i < original_count; i++)
    {
        if (original_tokens[i].distance != decoded_tokens[i].distance ||
            original_tokens[i].length != decoded_tokens[i].length ||
            original_tokens[i].next_byte != decoded_tokens[i].next_byte ||
            original_tokens[i].has_match != decoded_tokens[i].has_match ||
            original_tokens[i].has_next_byte != decoded_tokens[i].has_next_byte)
        {
            goto error;
        }
    }

    printf("PASS: Original and reconstructed tokens match.\n");

    /* Step 5: Decompress reconstructed tokens */
    if (lz77_decompress(decoded_tokens, decoded_count,
                        &reconstructed, &reconstructed_size) != 0)
        goto error;

    printf("PASS: Reconstructed tokens successfully decompressed.\n");

    /* Step 6: Verify original data */
    if (reconstructed_size != input_size ||
        memcmp(input, reconstructed, input_size) != 0)
        goto error;

    printf("PASS: Original and reconstructed data match.\n");

    printf("\nALL TOKEN CODEC TESTS PASSED!\n");

    free(original_tokens);
    free(serialized);
    free(decoded_tokens);
    free(reconstructed);

    return 0;

error:

    printf("\nFAIL: Token codec test.\n");

    free(original_tokens);
    free(serialized);
    free(decoded_tokens);
    free(reconstructed);

    return 1;
}