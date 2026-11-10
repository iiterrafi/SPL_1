#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "compressor.h"

static int run_test(
    const char *name,
    const uint8_t *input,
    size_t input_size)
{
    printf("\nTesting: %s\n", name);

    uint8_t *compressed = NULL;
    size_t compressed_size = 0;

    uint8_t *decompressed = NULL;
    size_t decompressed_size = 0;

    CompressorMetadata metadata;

    /* Compression */
    if (compressor_compress(
            input,
            input_size,
            &compressed,
            &compressed_size,
            &metadata,
            64) != 0)
    {
        printf("FAIL: Compression failed.\n");
        return -1;
    }

    printf("Original size:      %zu bytes\n", input_size);
    printf("Compressed size:    %zu bytes\n", compressed_size);

    /* Decompression */
    if (compressor_decompress(
            compressed,
            compressed_size,
            &metadata,
            &decompressed,
            &decompressed_size) != 0)
    {
        printf("FAIL: Decompression failed.\n");
        free(compressed);
        return -1;
    }

    /* Compare original and reconstructed data */
    if (input_size != decompressed_size ||
        (input_size > 0 &&
         memcmp(input, decompressed, input_size) != 0))
    {
        printf("FAIL: Data mismatch.\n");
        free(compressed);
        free(decompressed);
        return -1;
    }

    printf("PASS: Compression and decompression.\n");
    printf("PASS: Original data matches reconstructed data.\n");
    printf("PASS: CRC32 integrity verification.\n");

    free(compressed);
    free(decompressed);

    return 0;
}

int main(void)
{
    const uint8_t text[] =
        "This is a simple test of our hybrid lossless "
        "compression system. Compression must preserve "
        "every original byte.";

    const uint8_t repetitive[] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC";

    const uint8_t binary[] = {
        0x00, 0xFF, 0x01, 0x80,
        0x00, 0xFF, 0x7F, 0xAA,
        0x55, 0x00, 0x10, 0x20,
        0x30, 0x40, 0x50, 0x60
    };

    int failures = 0;

    if (run_test("Ordinary Text",
                 text, sizeof(text) - 1) != 0)
        failures++;

    if (run_test("Repetitive Data",
                 repetitive, sizeof(repetitive) - 1) != 0)
        failures++;

    if (run_test("Binary Data",
                 binary, sizeof(binary)) != 0)
        failures++;

    if (run_test("Empty Input",
                 NULL, 0) != 0)
        failures++;

    printf("\n====================================\n");

    if (failures == 0)
        printf("ALL COMPRESSOR TESTS PASSED!\n");
    else
        printf("%d COMPRESSOR TEST(S) FAILED!\n", failures);

    printf("====================================\n");

    return failures == 0 ? 0 : 1;
}