#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rle.h"
#include "lz77.h"

int main(){
    /* Original test data */
    uint8_t input[] =
        "AAAAABBBBBBBBBCCCCCCCC"
        "ABCABCABCABCABCABC"
        "DDDDDDDDDD"
        "XYZXYZXYZXYZ";

    size_t input_size = sizeof(input) - 1;

    /* RLE output */
    uint8_t *rle_output = NULL;
    size_t rle_output_size = 0;

    /* LZ77 output */
    LZ77Token *lz77_tokens = NULL;
    size_t token_count = 0;

    /* Reconstructed intermediate data */
    uint8_t *lz77_output = NULL;
    size_t lz77_output_size = 0;

    /* Final reconstructed data */
    uint8_t *final_output = NULL;
    size_t final_output_size = 0;

    printf("Testing RLE + LZ77 Integration...\n\n");

    /* Step 1: RLE Compression */
    if (rle_compress(input, input_size, &rle_output, &rle_output_size) != 0){
        printf("FAIL: RLE compression.\n");
        goto cleanup;
    }

    printf("PASS: RLE compression.\n");
    printf("Original size: %zu bytes\n", input_size);
    printf("After RLE:     %zu bytes\n\n", rle_output_size);

    /* Step 2: LZ77 Compression */
    if (lz77_compress(rle_output, rle_output_size, &lz77_tokens, &token_count, 16) != 0){
        printf("FAIL: LZ77 compression.\n");
        goto cleanup;
    }

    printf("PASS: LZ77 compression.\n");
    printf("LZ77 tokens: %zu\n\n", token_count);

    /* Step 3: LZ77 Decompression */
    if (lz77_decompress(lz77_tokens, token_count, &lz77_output, &lz77_output_size) != 0){
        printf("FAIL: LZ77 decompression.\n");
        goto cleanup;
    }

    printf("PASS: LZ77 decompression.\n");

    /* Step 4: RLE Decompression */
    if (rle_decompress(lz77_output, lz77_output_size, &final_output, &final_output_size) != 0){
        printf("FAIL: RLE decompression.\n");
        goto cleanup;
    }

    printf("PASS: RLE decompression.\n");

    /* Step 5: Verify original data */
    if (final_output_size != input_size || memcmp(input, final_output, input_size) != 0){
        printf("FAIL: Reconstructed data does not match.\n");
        goto cleanup;
    }

    printf("PASS: Original and reconstructed data match.\n");

    printf("\nALL RLE + LZ77 INTEGRATION TESTS PASSED!\n");

    free(rle_output);
    free(lz77_tokens);
    free(lz77_output);
    free(final_output);

    return 0;

cleanup:

    free(rle_output);
    free(lz77_tokens);
    free(lz77_output);
    free(final_output);

    return 1;
}