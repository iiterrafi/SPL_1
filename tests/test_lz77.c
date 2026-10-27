#include "lz77.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_empty_input();
static int test_single_byte_input();
static int test_repeated_data();
static int test_overlapping_match();
static int test_binary_data();
static int test_invalid_arguments();
static int test_different_chain_lengths();
static int test_invalid_match_distance();
static int test_invalid_match_length();
static int test_invalid_boolean_field();
static int test_invalid_literal_token();
static int test_excessive_match_length();
static int test_maximum_match_length();
static int test_maximum_window_distance();
static int test_missing_token_data();

int main(){
    const uint8_t input[] = "ABCABCX";
    const size_t input_size = sizeof(input) - 1;
    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("Testing LZ77 compression and decompression...\n");

    /* Compress the input */
    if (lz77_compress(input, input_size, &tokens, &token_count, 64)!= 0){
        printf("FAIL: Compression failed.\n");
        return 1;
    }

    printf("PASS: Compression succeeded.\n");
    printf("Original size: %zu bytes\n", input_size);
    printf("Token count:   %zu\n", token_count);

    /* Decompress the tokens */
    if(lz77_decompress(tokens, token_count, &output, &output_size) != 0) {
        printf("FAIL: Decompression failed.\n");
        free(tokens);
        return 1;
    }

    printf("PASS: Decompression succeeded.\n");

    /* Verify that the original bytes were restored */
    if (output_size != input_size || memcmp(input, output, input_size) != 0) {
        printf("FAIL: Reconstructed data differs from input.\n");

        free(tokens);
        free(output);

        return 1;
    }

    printf("PASS: Original data restored correctly.\n");

    free(tokens);
    free(output);

    /* Run the empty-input test */
    if (!test_empty_input()) {
        return 1;
    }

    /* Run the single-byte test */
    if (!test_single_byte_input()) {
        return 1;
    }

    /* Run the repeated-data test */
    if (!test_repeated_data()) {
        return 1;
    }

    /* Run the overlapping-match test */
    if (!test_overlapping_match()) {
        return 1;
    }

    /* Run the binary-data test */
    if (!test_binary_data()) {
        return 1;
    }


    /* Run the invalid-argument test */
    if (!test_invalid_arguments()) {
        return 1;
    }

    /* Run the different hash-chain lengths test */
    if (!test_different_chain_lengths()) {
        return 1;
    }

    /* Run the invalid match-distance test */
    if (!test_invalid_match_distance()) {
        return 1;
    }

    /* Run the invalid match-length test */
    if (!test_invalid_match_length()) {
        return 1;
    }
    
    /* Run the invalid Boolean-field test */
    if (!test_invalid_boolean_field()) {
        return 1;
    }

    /* Run the maximum match-length test */
    if (!test_maximum_match_length()) {
        return 1;
    }

    /* Run the invalid literal-token test */
    if (!test_invalid_literal_token()) {
        return 1;
    }

    /* Run the excessive match-length test */
    if (!test_excessive_match_length()) {
        return 1;
    }

    if (!test_maximum_window_distance()) {
        return 1;
    }

    if (!test_missing_token_data()) {
        return 1;
    }

    printf("\nALL LZ77 TESTS PASSED!\n");
    
    return 0;
}

/* Test compression and decompression of empty input */
static int test_empty_input(){
    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting empty input...\n");

    /* Compress empty input */
    if (lz77_compress(NULL, 0, &tokens, &token_count, 64) != 0) {
        printf("FAIL: Empty-input compression failed.\n");
        return 0;
    }

    if (tokens != NULL || token_count != 0) {
        printf("FAIL: Empty input produced unexpected tokens.\n");
        free(tokens);
        return 0;
    }

    /* Decompress the empty token sequence */
    if (lz77_decompress(tokens, token_count, &output, &output_size) != 0) {
        printf("FAIL: Empty-input decompression failed.\n");
        free(tokens);
        return 0;
    }

    if (output != NULL || output_size != 0) {
        printf("FAIL: Empty input produced unexpected output.\n");
        free(tokens);
        free(output);
        return 0;
    }

    printf("PASS: Empty input handled correctly.\n");

    free(tokens);
    free(output);

    return 1;

}

/* Test compression and decompression of a single byte */
static int test_single_byte_input() {
    const uint8_t input[] = {'A'};
    const size_t input_size = sizeof(input);

    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting single-byte input...\n");

    /* Compress the single byte */
    if (lz77_compress(input, input_size, &tokens, &token_count, 64) != 0) {
        printf("FAIL: Single-byte compression failed.\n");
        return 0;
    }

    /* One byte should produce one literal-only token */
    if (token_count != 1 || tokens[0].has_match != 0 || tokens[0].has_next_byte != 1 || tokens[0].next_byte != 'A') {
        printf("FAIL: Single byte was not stored as expected.\n");
        free(tokens);
        return 0;
    }

    /* Decompress the token */
    if (lz77_decompress(tokens, token_count, &output, &output_size) != 0) {
        printf("FAIL: Single-byte decompression failed.\n");
        free(tokens);
        return 0;
    }

    /* Verify the restored byte */
    if (output_size != input_size ||
        memcmp(input, output, input_size) != 0) {
        printf("FAIL: Single-byte output differs from input.\n");
        free(tokens);
        free(output);
        return 0;
    }

    printf("PASS: Single-byte input handled correctly.\n");

    free(tokens);
    free(output);

    return 1;

}


/* Test compression and decompression of repeated data */
static int test_repeated_data(){
    const uint8_t input[] = "ABCABCABCABC";
    const size_t input_size = sizeof(input) - 1;

    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    int match_found = 0;

    printf("\nTesting repeated data...\n");

    /* Compress the repeated input */
    if (lz77_compress(input, input_size, &tokens, &token_count, 64) != 0) {
        printf("FAIL: Repeated-data compression failed.\n");
        return 0;
    }

    /* Check whether the compressor produced a match */
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].has_match) {
            match_found = 1;
            break;
        }
    }

    if (!match_found) {
        printf("FAIL: No LZ77 match found in repeated data.\n");
        free(tokens);
        return 0;
    }

    printf("PASS: LZ77 match found.\n");

    /* Decompress the tokens */
    if (lz77_decompress(tokens, token_count, &output, &output_size) != 0) {
        printf("FAIL: Repeated-data decompression failed.\n");
        free(tokens);
        return 0;
    }

    /* Verify that the original data was restored */
    if (output_size != input_size ||
        memcmp(input, output, input_size) != 0) {
        printf("FAIL: Reconstructed repeated data differs from input.\n");
        free(tokens);
        free(output);
        return 0;
    }

    printf("PASS: Repeated data restored correctly.\n");

    free(tokens);
    free(output);

    return 1;

}

/* Test LZ77 overlapping matches using repeated bytes */
static int test_overlapping_match() {
    const uint8_t input[] = "AAAAAAAAAA";
    const size_t input_size = sizeof(input) - 1;

    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    int overlapping_match_found = 0;

    printf("\nTesting overlapping matches...\n");

    /* Compress the repeated input */
    if (lz77_compress(input, input_size, &tokens, &token_count, 64) != 0) {
        printf("FAIL: Overlapping-match compression failed.\n");
        return 0;
    }

    /* Look for a match whose length exceeds its distance */
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].has_match &&
            tokens[i].distance > 0 &&
            tokens[i].length > tokens[i].distance) {
            overlapping_match_found = 1;
            break;
        }
    }

    if (!overlapping_match_found) {
        printf("FAIL: No overlapping LZ77 match found.\n");
        free(tokens);
        return 0;
    }

    printf("PASS: Overlapping LZ77 match found.\n");

    /* Decompress the tokens */
    if (lz77_decompress(tokens, token_count, &output, &output_size) != 0) {
        printf("FAIL: Overlapping-match decompression failed.\n");
        free(tokens);
        return 0;
    }

    /* Verify the reconstructed bytes */
    if (output_size != input_size || memcmp(input, output, input_size) != 0) {
        printf("FAIL: Overlapping-match output differs from input.\n");
        free(tokens);
        free(output);
        return 0;
    }

    printf("PASS: Overlapping-match data restored correctly.\n");

    free(tokens);
    free(output);

    return 1;

}

/* Test compression and decompression of binary data */
static int test_binary_data() {
    const uint8_t input[] = {0, 255, 128, 1, 0, 255, 128, 1};
    const size_t input_size = sizeof(input);

    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting binary data...\n");

    /* Compress the binary input */
    if (lz77_compress(input, input_size, &tokens, &token_count, 64) != 0) {
        printf("FAIL: Binary-data compression failed.\n");
        return 0;
    }

    printf("PASS: Binary-data compression succeeded.\n");

    /* Decompress the tokens */
    if (lz77_decompress(tokens, token_count, &output, &output_size) != 0) {
        printf("FAIL: Binary-data decompression failed.\n");
        free(tokens);
        return 0;
    }

    /* Verify the restored bytes */
    if (output_size != input_size ||
        memcmp(input, output, input_size) != 0) {
        printf("FAIL: Reconstructed binary data differs from input.\n");
        free(tokens);
        free(output);
        return 0;
    }

    printf("PASS: All binary bytes restored correctly.\n");

    free(tokens);
    free(output);

    return 1;

}

/* Test whether LZ77 rejects invalid arguments */
static int test_invalid_arguments() {
    const uint8_t input[] = {'A', 'B', 'C'};

    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting invalid arguments...\n");

    /* Test 1: NULL input with nonzero size */
    if (lz77_compress(NULL, 3, &tokens, &token_count, 64) != -1) {
        printf("FAIL: NULL input was not rejected.\n");
        free(tokens);
        return 0;
    }
    printf("PASS: NULL input rejected.\n");

    /* Test 2: NULL token output pointer */
    if (lz77_compress(input, sizeof(input), NULL, &token_count, 64) != -1) {
        printf("FAIL: NULL token output pointer was not rejected.\n");
        return 0;
    }
    printf("PASS: NULL token output pointer rejected.\n");

    /* Test 3: NULL token-count pointer */
    if (lz77_compress(input, sizeof(input), &tokens, NULL, 64) != -1) {
        printf("FAIL: NULL token-count pointer was not rejected.\n");
        free(tokens);
        return 0;
    }
    printf("PASS: NULL token-count pointer rejected.\n");

    /* Test 4: Zero maximum chain length */
    if (lz77_compress(input, sizeof(input), &tokens, &token_count, 0) != -1) {
        printf("FAIL: Zero chain length was not rejected.\n");
        free(tokens);
        return 0;
    }
    printf("PASS: Zero chain length rejected.\n");

    /* Test 5: NULL decompression output pointer */
    if (lz77_decompress(NULL, 0, NULL, &output_size) != -1) {
        printf("FAIL: NULL decompression output pointer was not rejected.\n");
        return 0;
    }
    printf("PASS: NULL decompression output pointer rejected.\n");

    /* Test 6: NULL decompression output-size pointer */
    if (lz77_decompress(NULL, 0, &output, NULL) != -1) {
        printf("FAIL: NULL output-size pointer was not rejected.\n");
        free(output);
        return 0;
    }
    printf("PASS: NULL output-size pointer rejected.\n");

    /* Test 7: NULL token array with nonzero count */
    if (lz77_decompress(NULL, 1, &output, &output_size) != -1) {
        printf("FAIL: NULL token array was not rejected.\n");
        free(output);
        return 0;
    }

    printf("PASS: NULL token array rejected.\n");

    return 1;

}

/* Test LZ77 with different hash-chain lengths */
static int test_different_chain_lengths() {
    const uint8_t input[] = "ABCABCABCABCABCABCXYZXYZXYZABCABC";
    const size_t input_size = sizeof(input) - 1;

    const size_t chain_lengths[] = {1, 16, 64};
    const size_t number_of_lengths =
        sizeof(chain_lengths) / sizeof(chain_lengths[0]);

    printf("\nTesting different hash-chain lengths...\n");

    for (size_t i = 0; i < number_of_lengths; i++) {
        LZ77Token *tokens = NULL;
        size_t token_count = 0;

        uint8_t *output = NULL;
        size_t output_size = 0;

        size_t chain_length = chain_lengths[i];

        /* Compress using the current chain length */
        if (lz77_compress(input, input_size, &tokens, &token_count, chain_length) != 0) {
            printf("FAIL: Compression failed with chain length %zu.\n", chain_length);
            free(tokens);
            return 0;
        }

        /* Decompress the generated tokens */
        if (lz77_decompress(tokens, token_count, &output, &output_size) != 0) {
            printf("FAIL: Decompression failed with chain length %zu.\n", chain_length);
            free(tokens);
            free(output);
            return 0;
        }

        /* Verify that the original data was restored */
        if (output_size != input_size || memcmp(input, output, input_size) != 0) {
            printf("FAIL: Data mismatch with chain length %zu.\n", chain_length);
            free(tokens);
            free(output);
            return 0;
        }

        printf("PASS: Chain length %zu restored the original data.\n", chain_length);

        free(tokens);
        free(output);
    }

    return 1;

}

/* Test whether the decompressor rejects an invalid match distance */
static int test_invalid_match_distance() {
    LZ77Token token = {
        .distance = 1,
        .length = 3,
        .next_byte = 0,
        .has_match = 1,
        .has_next_byte = 0
    };

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting invalid match distance...\n");

    if (lz77_decompress(&token, 1, &output, &output_size) != -1) {
        printf("FAIL: Invalid match distance was not rejected.\n");
        free(output);
        return 0;
    }

    printf("PASS: Invalid match distance rejected.\n");

    return 1;
}

/* Test whether the decompressor rejects an invalid match length */
static int test_invalid_match_length() {
    LZ77Token token = {
        .distance = 1,
        .length = 2,
        .next_byte = 0,
        .has_match = 1,
        .has_next_byte = 0
    };

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting invalid match length...\n");

    if (lz77_decompress(&token, 1, &output, &output_size) != -1) {
        printf("FAIL: Invalid match length was not rejected.\n");
        free(output);
        return 0;
    }

    printf("PASS: Invalid match length rejected.\n");

    return 1;

}

/* Test whether the decompressor rejects invalid Boolean fields */
static int test_invalid_boolean_field(void) {
    LZ77Token token = {
        .distance = 0,
        .length = 0,
        .next_byte = 'A',
        .has_match = 2,
        .has_next_byte = 1
    };

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting invalid Boolean field...\n");

    if (lz77_decompress(&token, 1, &output, &output_size) != -1) {
        printf("FAIL: Invalid Boolean field was not rejected.\n");
        free(output);
        return 0;
    }

    printf("PASS: Invalid Boolean field rejected.\n");

    return 1;

}

/* Test whether the decompressor rejects an invalid literal token */
static int test_invalid_literal_token(void) {
    LZ77Token token = {
        .distance = 0,
        .length = 3,
        .next_byte = 'A',
        .has_match = 0,
        .has_next_byte = 1
    };

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting invalid literal token...\n");

    if (lz77_decompress(&token, 1, &output, &output_size) != -1) {
        printf("FAIL: Invalid literal token was not rejected.\n");
        free(output);
        return 0;
    }

    printf("PASS: Invalid literal token rejected.\n");

    return 1;

}

/* Test whether the decompressor rejects a match exceeding the maximum length */
static int test_excessive_match_length(void) {
    LZ77Token token = {
        .distance = 1,
        .length = 259,
        .next_byte = 0,
        .has_match = 1,
        .has_next_byte = 0
    };

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting excessive match length...\n");

    if (lz77_decompress(&token, 1, &output, &output_size) != -1) {
        printf("FAIL: Excessive match length was not rejected.\n");
        free(output);
        return 0;
    }

    printf("PASS: Excessive match length rejected.\n");

return 1;

}

/* Test whether the decompressor accepts the maximum valid match length */
static int test_maximum_match_length() {
    uint8_t original[258];

    /* Fill the original data with the same byte */
    for (size_t i = 0; i < sizeof(original); i++) {
        original[i] = 'A';
    }

    /*
     * The first 3 bytes are already available.
     * The match copies 258 bytes from distance 1,
     * producing a total of 261 bytes.
     */
    LZ77Token tokens[] = {
        {
            .distance = 0,
            .length = 0,
            .next_byte = 'A',
            .has_match = 0,
            .has_next_byte = 1
        },
        {
            .distance = 1,
            .length = 258,
            .next_byte = 'A',
            .has_match = 1,
            .has_next_byte = 1
        }
    };

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting maximum match length...\n");

    if (lz77_decompress(
            tokens,
            sizeof(tokens) / sizeof(tokens[0]),
            &output,
            &output_size) != 0) {
        printf("FAIL: Maximum valid match length was rejected.\n");
        free(output);
        return 0;
    }

    if (output_size != 260) {
        printf("FAIL: Unexpected output size: %zu bytes.\n", output_size);
        free(output);
        return 0;
    }

    for (size_t i = 0; i < output_size; i++) {
        if (output[i] != 'A') {
            printf("FAIL: Incorrect byte at position %zu.\n", i);
            free(output);
            return 0;
        }
    }

    printf("PASS: Maximum valid match length accepted.\n");

    free(output);
    return 1;
}

/* Test whether the decompressor accepts the maximum window distance */
static int test_maximum_window_distance(){
    size_t distance = LZ77_WINDOW_SIZE;
    size_t token_count = distance + 1;

    LZ77Token *tokens = calloc(token_count, sizeof(LZ77Token));

    if (tokens == NULL) {
        printf("FAIL: Could not allocate test tokens.\n");
        return 0;
    }

    /*
     * Create enough literal tokens to produce
     * LZ77_WINDOW_SIZE bytes of output.
     */
    for (size_t i = 0; i < distance; i++) {
        tokens[i].next_byte = 'A';
        tokens[i].has_match = 0;
        tokens[i].has_next_byte = 1;
    }

    /*
     * The final token refers to a byte exactly
     * LZ77_WINDOW_SIZE bytes behind the current position.
     */
    tokens[distance].distance = distance;
    tokens[distance].length = LZ77_MIN_MATCH;
    tokens[distance].has_match = 1;
    tokens[distance].has_next_byte = 0;

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting maximum window distance...\n");

    if (lz77_decompress(
            tokens,
            token_count,
            &output,
            &output_size) != 0) {
        printf("FAIL: Maximum window distance was rejected.\n");
        free(tokens);
        free(output);
        return 0;
    }

    if (output_size != distance + LZ77_MIN_MATCH) {
        printf("FAIL: Unexpected output size: %zu bytes.\n", output_size);
        free(tokens);
        free(output);
        return 0;
    }

    printf("PASS: Maximum window distance accepted.\n");

    free(tokens);
    free(output);

    return 1;
}

/* Test whether a token with no match and no literal is rejected */
static int test_missing_token_data(void) {
    LZ77Token token = {
        .distance = 0,
        .length = 0,
        .next_byte = 0,
        .has_match = 0,
        .has_next_byte = 0
    };

    uint8_t *output = NULL;
    size_t output_size = 0;

    printf("\nTesting token with missing data...\n");

    if (lz77_decompress(&token, 1, &output, &output_size) != -1) {
        printf("FAIL: Token with missing data was not rejected.\n");
        free(output);
        return 0;
    }

    printf("PASS: Token with missing data rejected.\n");

    return 1;
}