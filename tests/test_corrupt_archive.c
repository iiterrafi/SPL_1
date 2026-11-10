#include <stdio.h>
#include <unistd.h>

#include "file_compression.h"

static int test_corrupt(
    const char *name,
    const char *archive,
    const char *output
)
{
    remove(output);

    printf("\nTesting: %s\n", name);

    int result = decompress_archive_to_file(archive, output);

    if (result != 0 && access(output, F_OK) != 0)
    {
        printf("PASS: Damaged archive rejected safely.\n");
        return 0;
    }

    printf("FAIL: Damaged archive was accepted or output was created.\n");
    remove(output);
    return -1;
}

int main(void)
{
    int failures = 0;

    if (test_corrupt("Invalid signature",
        "test_data/corrupt_magic.cmp",
        "test_data/corrupt_magic_output.bin") != 0)
        failures++;

    if (test_corrupt("Truncated archive",
        "test_data/corrupt_truncated.cmp",
        "test_data/corrupt_truncated_output.bin") != 0)
        failures++;

    if (test_corrupt("Corrupted payload",
        "test_data/corrupt_payload.cmp",
        "test_data/corrupt_payload_output.bin") != 0)
        failures++;

    printf("\n===============================\n");

    if (failures == 0)
        printf("ALL CORRUPTION TESTS PASSED!\n");
    else
        printf("CORRUPTION TEST FAILURES: %d\n", failures);

    printf("===============================\n");

    return failures == 0 ? 0 : 1;
}
