#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "huffman.h"

/* Test ordinary frequency counting */
static int test_basic_frequencies() {
    const uint8_t input[] = "AABCA";
    const size_t input_size = sizeof(input) - 1;

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT];

    printf("\nTesting basic frequency counting...\n");

    if (huffman_count_frequencies(
            input, input_size, frequencies) != 0) {
        printf("FAIL: Frequency counting failed.\n");
        return 0;
    }

    if (frequencies['A'] != 3 ||
        frequencies['B'] != 1 ||
        frequencies['C'] != 1) {
        printf("FAIL: Incorrect frequency counts.\n");
        return 0;
    }

    printf("PASS: A=3, B=1, C=1.\n");
    return 1;
}

/* Test empty input */
static int test_empty_input() {
    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT];

    printf("\nTesting empty input...\n");

    if (huffman_count_frequencies(
            NULL, 0, frequencies) != 0) {
        printf("FAIL: Empty input was rejected.\n");
        return 0;
    }

    for (size_t i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        if (frequencies[i] != 0) {
            printf("FAIL: Frequency array was not cleared.\n");
            return 0;
        }
    }

    printf("PASS: Empty input produces zero frequencies.\n");
    return 1;
}

/* Test binary data */
static int test_binary_data() {
    const uint8_t input[] = {0, 255, 0, 128, 255, 0};
    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT];

    printf("\nTesting binary data...\n");

    if (huffman_count_frequencies(
            input, sizeof(input), frequencies) != 0) {
        printf("FAIL: Binary frequency counting failed.\n");
        return 0;
    }

    if (frequencies[0] != 3 ||
        frequencies[255] != 2 ||
        frequencies[128] != 1) {
        printf("FAIL: Incorrect binary frequencies.\n");
        return 0;
    }

    printf("PASS: Binary byte frequencies are correct.\n");
    return 1;
}

/* Test invalid arguments */
static int test_invalid_arguments() {
    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT];
    const uint8_t input[] = {'A'};

    printf("\nTesting invalid arguments...\n");

    if (huffman_count_frequencies(
            NULL, 1, frequencies) != -1) {
        printf("FAIL: NULL input was not rejected.\n");
        return 0;
    }

    if (huffman_count_frequencies(
            input, sizeof(input), NULL) != -1) {
        printf("FAIL: NULL frequency array was not rejected.\n");
        return 0;
    }

    printf("PASS: Invalid arguments rejected.\n");
    return 1;
}

int main() {
    printf("Testing Huffman frequency counting...\n");

    if (!test_basic_frequencies()) {
        return 1;
    }

    if (!test_empty_input()) {
        return 1;
    }

    if (!test_binary_data()) {
        return 1;
    }

    if (!test_invalid_arguments()) {
        return 1;
    }

    printf("\nALL HUFFMAN FREQUENCY TESTS PASSED!\n");
    return 0;
}