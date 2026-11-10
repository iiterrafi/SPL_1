#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "file_io.h"
#include "file_compression.h"
#include "archive.h"

int main(void)
{
    printf("\n====================================\n");
    printf(" MULTIPLE FILE COMPRESSION TEST\n");
    printf("====================================\n");

    const char *paths[] = {
        "test_data/multi_a.txt",
        "test_data/multi_b.csv",
        "test_data/multi_c.bin"
    };

    const uint8_t text_a[] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    const uint8_t text_b[] =
        "Name,Age,Department\nRafi,21,CSE\nHasan,22,EEE\n";

    uint8_t binary[256];

    for (size_t i = 0; i < 256; i++)
        binary[i] = (uint8_t)i;

    if (write_file(paths[0], text_a, sizeof(text_a) - 1) != 0 ||
        write_file(paths[1], text_b, sizeof(text_b) - 1) != 0 ||
        write_file(paths[2], binary, sizeof(binary)) != 0)
    {
        printf("FAIL: Could not create test files.\n");
        return 1;
    }

    printf("\nOriginal files created successfully.\n");

    if (compress_files_to_archive(
            paths, 3, "test_data/multiple.cmp", 64) != 0)
    {
        printf("FAIL: Multiple-file compression.\n");
        return 1;
    }

    ArchiveEntry *entries = NULL;
    size_t entry_count = 0;

    if (archive_read(
            "test_data/multiple.cmp",
            &entries,
            &entry_count) != 0)
    {
        printf("FAIL: Could not read generated archive.\n");
        return 1;
    }

    if (entry_count != 3)
    {
        printf("FAIL: Expected 3 entries, found %zu.\n",
               entry_count);

        archive_free_entries(entries, entry_count);
        return 1;
    }

    printf("\nPASS: Archive contains exactly 3 entries.\n");

    for (size_t i = 0; i < 3; i++)
    {
        if (entries[i].type != ARCHIVE_ENTRY_FILE ||
            strcmp(entries[i].path, paths[i]) != 0)
        {
            printf("FAIL: Entry %zu is incorrect.\n", i);

            archive_free_entries(entries, entry_count);
            return 1;
        }

        printf("PASS: %s | %llu bytes\n",
               entries[i].path,
               (unsigned long long)entries[i].original_size);
    }

    archive_free_entries(entries, entry_count);

    printf("\n====================================\n");
    printf("MULTIPLE FILE COMPRESSION PASSED!\n");
    printf("====================================\n");

    return 0;
}
