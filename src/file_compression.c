#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#include "file_compression.h"
#include "file_io.h"
#include "compressor.h"
#include "archive.h"
#include "crc32.h"

/* Compress one file into an archive. */
int compress_file_to_archive(
    const char *input_path,
    const char *archive_path,
    size_t max_chain_length)
{
    if (!input_path || !archive_path || max_chain_length == 0)
        return -1;

    FileBuffer original = {NULL, 0};

    uint8_t *compressed = NULL;
    size_t compressed_size = 0;

    CompressorMetadata metadata = {0};

    ArchiveEntry entry = {0};

    if (read_file(input_path, &original) != 0)
        goto failure;

    if (compressor_compress(
            original.data,
            original.size,
            &compressed,
            &compressed_size,
            &metadata,
            max_chain_length) != 0)
        goto failure;

    uint32_t checksum;

    if (crc32_calculate(
            original.data,
            original.size,
            &checksum) != 0)
        goto failure;

    entry.path = (char *)input_path;
    entry.type = ARCHIVE_ENTRY_FILE;
    entry.original_size = original.size;
    entry.crc32 = checksum;

    /* Use HYBRID only when it actually saves space. */
    if (compressed_size < original.size)
    {
        entry.method = ARCHIVE_METHOD_HYBRID;
        entry.payload = compressed;
        entry.payload_size = compressed_size;
        entry.metadata = metadata;
    }
    else
    {
        entry.method = ARCHIVE_METHOD_RAW;
        entry.payload = original.data;
        entry.payload_size = original.size;
    }

    if (archive_write(archive_path, &entry, 1) != 0)
        goto failure;

    printf("Archive created successfully.\n");
    printf("Original size:   %zu bytes\n", original.size);
    printf("Stored size:     %zu bytes\n", entry.payload_size);
    printf("Storage method:  %s\n",
           entry.method == ARCHIVE_METHOD_HYBRID ? "HYBRID" : "RAW");

    free(compressed);
    free_file_buffer(&original);

    return 0;

failure:
    free(compressed);
    free_file_buffer(&original);

    fprintf(stderr, "File compression failed.\n");

    return -1;
}


/* Decompress one archive entry into a file. */
int decompress_archive_to_file(
    const char *archive_path,
    const char *output_path)
{
    if (!archive_path || !output_path)
        return -1;

    ArchiveEntry *entries = NULL;
    size_t entry_count = 0;

    uint8_t *restored = NULL;
    size_t restored_size = 0;

    if (archive_read(
            archive_path,
            &entries,
            &entry_count) != 0)
        goto failure;

    /* This initial version handles one file per archive. */
    if (entry_count != 1 ||
        entries[0].type != ARCHIVE_ENTRY_FILE ||
        entries[0].original_size > SIZE_MAX)
        goto failure;

    ArchiveEntry *entry = &entries[0];

    if (entry->method == ARCHIVE_METHOD_RAW)
    {
        if (entry->payload_size != entry->original_size)
            goto failure;

        restored_size = entry->payload_size;

        if (restored_size > 0)
        {
            restored = malloc(restored_size);

            if (!restored)
                goto failure;

            for (size_t i = 0; i < restored_size; i++)
                restored[i] = entry->payload[i];
        }
    }
    else if (entry->method == ARCHIVE_METHOD_HYBRID)
    {
        if (compressor_decompress(
                entry->payload,
                entry->payload_size,
                &entry->metadata,
                &restored,
                &restored_size) != 0)
            goto failure;
    }
    else
    {
        goto failure;
    }

    /* Verify the restored size. */
    if (restored_size != entry->original_size)
        goto failure;

    /* Verify CRC32 before writing the restored file. */
    uint32_t checksum;

    if (crc32_calculate(
            restored,
            restored_size,
            &checksum) != 0 ||
        checksum != entry->crc32)
        goto failure;

    if (write_file(
            output_path,
            restored,
            restored_size) != 0)
        goto failure;

    printf("File restored successfully.\n");
    printf("Restored size: %zu bytes\n", restored_size);
    printf("CRC32 verification: PASSED\n");

    free(restored);
    archive_free_entries(entries, entry_count);

    return 0;

failure:
    free(restored);
    archive_free_entries(entries, entry_count);

    fprintf(stderr, "File decompression failed.\n");

    return -1;
}
