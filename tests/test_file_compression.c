#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_io.h"
#include "file_compression.h"

int main(){

    printf("Testing Real File Compression and Decompression...\n\n");

    const char *original_path = "test_data/original.txt";
    const char *archive_path = "test_data/original.cmp";
    const char *restored_path = "test_data/restored.txt";

    /* Create repetitive test content. */
    const unsigned char original[] =
        "AAAAABBBBBCCCCCDDDDDEEEEE"
        "AAAAABBBBBCCCCCDDDDDEEEEE"
        "AAAAABBBBBCCCCCDDDDDEEEEE"
        "AAAAABBBBBCCCCCDDDDDEEEEE";

    size_t original_size = sizeof(original) - 1;

    /* Create original file. */
    if (write_file(original_path, original, original_size) != 0)
    {
        printf("FAIL: Could not create original file.\n");
        return 1;
    }

    printf("PASS: Original file created.\n");

    /* Compress original file. */
    if (compress_file_to_archive(original_path, archive_path, 64) != 0)
    {
        printf("FAIL: File compression failed.\n");
        return 1;
    }

    printf("PASS: File compressed into .cmp archive.\n");

    /* Decompress archive. */
    if (decompress_archive_to_file(archive_path, restored_path) != 0)
    {
        printf("FAIL: File decompression failed.\n");
        return 1;
    }

    printf("PASS: Archive decompressed.\n");

    /* Read restored file. */
    FileBuffer restored = {NULL, 0};

    if (read_file(restored_path, &restored) != 0)
    {
        printf("FAIL: Could not read restored file.\n");
        return 1;
    }

    /* Compare original and restored bytes. */
    if (restored.size != original_size ||
        memcmp(restored.data, original, original_size) != 0)
    {
        printf("FAIL: Restored file differs from original.\n");
        free_file_buffer(&restored);
        return 1;
    }

    printf("PASS: Original and restored files match byte-for-byte.\n");

    free_file_buffer(&restored);

    printf("\nALL REAL FILE COMPRESSION TESTS PASSED!\n");

    return 0;
}
