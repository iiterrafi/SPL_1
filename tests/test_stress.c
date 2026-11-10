#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "file_io.h"
#include "file_compression.h"

#define TEST_SIZE (1024 * 1024)

static uint32_t random_state = 123456789u;

static uint8_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;

    return (uint8_t)random_state;
}

static int run_test(
    const char *name,
    const uint8_t *data,
    size_t size,
    int test_number
)
{
    char original_path[128];
    char archive_path[128];
    char restored_path[128];

    snprintf(original_path, sizeof(original_path),
             "test_data/stress_%d_original.bin", test_number);

    snprintf(archive_path, sizeof(archive_path),
             "test_data/stress_%d.cmp", test_number);

    snprintf(restored_path, sizeof(restored_path),
             "test_data/stress_%d_restored.bin", test_number);

    printf("\n========== %s ==========\n", name);

    if (write_file(original_path, data, size) != 0)
    {
        printf("FAIL: Could not create original file.\n");
        return -1;
    }

    if (compress_file_to_archive(original_path, archive_path, 64) != 0)
    {
        printf("FAIL: Compression failed.\n");
        return -1;
    }

    if (decompress_archive_to_file(archive_path, restored_path) != 0)
    {
        printf("FAIL: Decompression failed.\n");
        return -1;
    }

    FileBuffer restored = {NULL, 0};
    FileBuffer archive = {NULL, 0};

    if (read_file(restored_path, &restored) != 0 ||
        read_file(archive_path, &archive) != 0)
    {
        printf("FAIL: Could not read generated files.\n");
        free_file_buffer(&restored);
        free_file_buffer(&archive);
        return -1;
    }

    printf("Original size: %zu bytes\n", size);
    printf("Archive size:  %zu bytes\n", archive.size);
    printf("Restored size: %zu bytes\n", restored.size);

    int passed = restored.size == size &&
                 (size == 0 || memcmp(data, restored.data, size) == 0);

    if (passed)
        printf("PASS: Exact byte-for-byte restoration.\n");
    else
        printf("FAIL: Restored data differs from original.\n");

    free_file_buffer(&restored);
    free_file_buffer(&archive);

    return passed ? 0 : -1;
}

int main(void)
{
    uint8_t *data = malloc(TEST_SIZE);

    if (!data)
    {
        printf("FAIL: Memory allocation.\n");
        return 1;
    }

    int failures = 0;

    /* Test 1: Empty file */
    if (run_test("EMPTY FILE", NULL, 0, 1) != 0)
        failures++;

    /* Test 2: Highly repetitive data */
    for (size_t i = 0; i < TEST_SIZE; i++)
        data[i] = (uint8_t)("ABCD"[i % 4]);

    if (run_test("1 MB REPETITIVE FILE", data, TEST_SIZE, 2) != 0)
        failures++;

    /* Test 3: Pseudorandom binary data */
    for (size_t i = 0; i < TEST_SIZE; i++)
        data[i] = next_random();

    if (run_test("1 MB RANDOM BINARY FILE", data, TEST_SIZE, 3) != 0)
        failures++;

    /* Test 4: Mixed repetitive and random data */
    for (size_t i = 0; i < TEST_SIZE; i++)
    {
        if (i % 4 == 0)
            data[i] = next_random();
        else
            data[i] = (uint8_t)("ABCD"[i % 4]);
    }

    if (run_test("1 MB MIXED FILE", data, TEST_SIZE, 4) != 0)
        failures++;

    free(data);

    printf("\n====================================\n");

    if (failures == 0)
        printf("ALL STRESS TESTS PASSED!\n");
    else
        printf("STRESS TEST FAILURES: %d\n", failures);

    printf("====================================\n");

    return failures == 0 ? 0 : 1;
}
