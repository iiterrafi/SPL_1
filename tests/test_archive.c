#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"

int main(){

    printf("Testing Archive Write and Read...\n\n");

    uint8_t raw_data[] = "This is a raw file.";
    uint8_t hybrid_data[] = {0x12, 0x34, 0x56, 0x78};

    ArchiveEntry entries[2] = {0};

    /* First entry: RAW */
    entries[0].path = "raw.txt";
    entries[0].type = ARCHIVE_ENTRY_FILE;
    entries[0].method = ARCHIVE_METHOD_RAW;
    entries[0].original_size = sizeof(raw_data) - 1;
    entries[0].crc32 = 0x12345678;
    entries[0].payload = raw_data;
    entries[0].payload_size = sizeof(raw_data) - 1;

    /* Second entry: HYBRID */
    entries[1].path = "compressed.bin";
    entries[1].type = ARCHIVE_ENTRY_FILE;
    entries[1].method = ARCHIVE_METHOD_HYBRID;
    entries[1].original_size = 100;
    entries[1].crc32 = 0xABCDEF12;
    entries[1].payload = hybrid_data;
    entries[1].payload_size = sizeof(hybrid_data);

    entries[1].metadata.original_size = 100;
    entries[1].metadata.rle_size = 80;
    entries[1].metadata.token_count = 20;
    entries[1].metadata.serialized_size = 120;
    entries[1].metadata.meaningful_bits = 32;
    entries[1].metadata.crc32 = 0xABCDEF12;

    /* Write archive */
    if (archive_write("test_archive.cmp", entries, 2) != 0)
    {
        printf("FAIL: Could not write archive.\n");
        return 1;
    }

    printf("PASS: Archive created successfully.\n");

    /* Read archive */
    ArchiveEntry *loaded_entries = NULL;
    size_t loaded_count = 0;

    if (archive_read("test_archive.cmp",
                 &loaded_entries,
                 &loaded_count) != 0)
    {
        printf("FAIL: Could not read archive.\n");
        return 1;
    }

    printf("PASS: Archive read successfully.\n");

    /* Verify entry count */
    if (loaded_count != 2)
    {
        printf("FAIL: Incorrect entry count.\n");
        archive_free_entries(loaded_entries, loaded_count);
        return 1;
    }

    printf("PASS: Entry count matches.\n");

    /* Verify first entry */
    if (strcmp(loaded_entries[0].path, "raw.txt") != 0 ||
        loaded_entries[0].method != ARCHIVE_METHOD_RAW ||
        loaded_entries[0].payload_size != sizeof(raw_data) - 1 ||
        memcmp(loaded_entries[0].payload,
               raw_data,
               sizeof(raw_data) - 1) != 0)
    {
        printf("FAIL: RAW entry verification failed.\n");
        archive_free_entries(loaded_entries, loaded_count);
        return 1;
    }

    printf("PASS: RAW entry matches original data.\n");

    /* Verify second entry */
    if (strcmp(loaded_entries[1].path, "compressed.bin") != 0 ||
        loaded_entries[1].method != ARCHIVE_METHOD_HYBRID ||
        loaded_entries[1].payload_size != sizeof(hybrid_data) ||
        memcmp(loaded_entries[1].payload,
               hybrid_data,
               sizeof(hybrid_data)) != 0)
    {
        printf("FAIL: HYBRID entry verification failed.\n");
        archive_free_entries(loaded_entries, loaded_count);
        return 1;
    }

    printf("PASS: HYBRID entry matches original data.\n");

    /* Verify metadata */
    if (loaded_entries[1].metadata.original_size != 100 ||
        loaded_entries[1].metadata.rle_size != 80 ||
        loaded_entries[1].metadata.token_count != 20 ||
        loaded_entries[1].metadata.meaningful_bits != 32 ||
        loaded_entries[1].metadata.crc32 != 0xABCDEF12)
    {
        printf("FAIL: Metadata verification failed.\n");
        archive_free_entries(loaded_entries, loaded_count);
        return 1;
    }

    printf("PASS: Compression metadata preserved.\n");

    archive_free_entries(loaded_entries, loaded_count);

    printf("\nALL ARCHIVE TESTS PASSED!\n");

    return 0;
}
