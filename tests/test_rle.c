#include "rle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test whether decompression restores the original input */
static int test_round_trip(const char *test_name, const uint8_t *input, size_t input_size){
uint8_t *compressed = NULL;
size_t compressed_size = 0;

uint8_t *decompressed = NULL;
size_t decompressed_size = 0;

if (rle_compress(input, input_size, &compressed, &compressed_size) != 0) {
    printf("FAIL: %s - compression failed.\n", test_name);
    return 0;
}

if (rle_decompress(compressed, compressed_size, &decompressed, &decompressed_size) != 0) {
    printf("FAIL: %s - decompression failed.\n", test_name);
    free(compressed);
    return 0;
}

int passed = decompressed_size == input_size && (input_size == 0 || memcmp(input, decompressed, input_size) == 0);

if (passed) {
    printf("PASS: %s\n", test_name);
    printf("      Original size: %zu bytes\n", input_size);
    printf("      Compressed size: %zu bytes\n", compressed_size);
} else {
    printf("FAIL: %s - original data was not restored.\n", test_name);
}

free(compressed);
free(decompressed);

return passed;

}

/* Test RLE with a generated input of a specified size */
static int test_generated_input(
const char *test_name,
size_t input_size,
uint8_t value,
int use_repeated_bytes
) {
uint8_t *input = NULL;

if (input_size > 0) {
    input = malloc(input_size);

    if (input == NULL) {
        printf("FAIL: %s - memory allocation failed.\n", test_name);
        return 0;
    }

    for (size_t i = 0; i < input_size; i++) {
        input[i] = use_repeated_bytes
            ? value
            : (uint8_t)(i % 251);
    }
}

int passed = test_round_trip(test_name, input, input_size);

free(input);

return passed;

}


int main(){
int passed = 1;

    /* Test 0: Reject invalid arguments */
    uint8_t *invalid_output = NULL;
    size_t invalid_output_size = 0;

    if (rle_compress(NULL, 5, &invalid_output,
                     &invalid_output_size) != -1) {
        printf("FAIL: Compression accepted a NULL input.\n");
        passed = 0;
    } else {
        printf("PASS: Compression rejects a NULL input.\n");
    }

    if (rle_compress(NULL, 0, NULL,
                     &invalid_output_size) != -1) {
        printf("FAIL: Compression accepted a NULL output pointer.\n");
        passed = 0;
    } else {
        printf("PASS: Compression rejects a NULL output pointer.\n");
    }

    if (rle_decompress(NULL, 5, &invalid_output,
                       &invalid_output_size) != -1) {
        printf("FAIL: Decompression accepted a NULL input.\n");
        passed = 0;
    } else {
        printf("PASS: Decompression rejects a NULL input.\n");
    }

/* Test 1: Repeated bytes */
const uint8_t repeated[] = {'A', 'A', 'A', 'A', 'A','B', 'B', 'B', 'C', 'C', 'C', 'C'};

passed &= test_round_trip("Repeated bytes", repeated, sizeof(repeated));

/* Test 2: Bytes that do not repeat consecutively */
const uint8_t ordinary[] = {'A', 'B', 'C', 'D', 'E', 'F'};

passed &= test_round_trip("Ordinary data", ordinary, sizeof(ordinary));

/* Test 3: Binary data, including zero and 255 */
const uint8_t binary[] = {0, 255, 0, 128, 255, 255, 255, 0};

passed &= test_round_trip("Binary data", binary, sizeof(binary));

/* Test 4: Empty input */
passed &= test_round_trip("Empty input", NULL, 0);

/* Test 5: A run longer than the maximum packet length */
uint8_t long_run[300];
memset(long_run, 'X', sizeof(long_run));

passed &= test_round_trip("Long repeated run", long_run, sizeof(long_run));

/* Test 6: Reject an incomplete literal packet */
const uint8_t bad_literal[] = {2, 'A'};

uint8_t *bad_output = NULL;
size_t bad_output_size = 0;

if (rle_decompress(
        bad_literal,
        sizeof(bad_literal),
        &bad_output,
        &bad_output_size) != -1) {
    printf("FAIL: Incomplete literal packet was accepted.\n");
    free(bad_output);
    passed = 0;
} else {
    printf("PASS: Incomplete literal packet rejected.\n");
}

/* Test 7: Reject a run packet without its repeated byte */
const uint8_t bad_run[] = {128};

if (rle_decompress(
        bad_run,
        sizeof(bad_run),
        &bad_output,
        &bad_output_size) != -1) {
    printf("FAIL: Incomplete run packet was accepted.\n");
    free(bad_output);
    passed = 0;
} else {
    printf("PASS: Incomplete run packet rejected.\n");
}

/* Test 8: Boundary cases for repeated runs */
passed &= test_generated_input("Run of 2 bytes", 2, 'A', 1);
passed &= test_generated_input("Run of 3 bytes", 3, 'A', 1);
passed &= test_generated_input("Run of 128 bytes", 128, 'A', 1);
passed &= test_generated_input("Run of 129 bytes", 129, 'A', 1);
passed &= test_generated_input("Run of 130 bytes", 130, 'A', 1);
passed &= test_generated_input("Run of 131 bytes", 131, 'A', 1);

/* Test 9: Boundary cases for literal packets */
passed &= test_generated_input("Literal data of 128 bytes", 128, 'A', 0);
passed &= test_generated_input("Literal data of 129 bytes", 129, 'A', 0);


if (!passed) {
    printf("\nSome RLE tests FAILED.\n");
    return 1;
}


printf("\nALL RLE TESTS PASSED!\n");
return 0;

}
