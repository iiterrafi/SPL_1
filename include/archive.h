#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdint.h>
#include <stddef.h>

#include "compressor.h"

/* Archive identification */
#define ARCHIVE_MAGIC "HCMP"
#define ARCHIVE_VERSION 1

/* Entry types */
#define ARCHIVE_ENTRY_FILE      1
#define ARCHIVE_ENTRY_DIRECTORY 2

/* Compression methods */
#define ARCHIVE_METHOD_RAW      0
#define ARCHIVE_METHOD_HYBRID   1


/* Information about one archive entry */
typedef struct {

    char *path;

    uint8_t type;
    uint8_t method;

    uint64_t original_size;

    uint32_t crc32;

    uint8_t *payload;
    size_t payload_size;

    CompressorMetadata metadata;

} ArchiveEntry;


/* Write multiple entries into an archive */
int archive_write(
    const char *archive_path,
    const ArchiveEntry *entries,
    size_t entry_count
);


/* Read entries from an archive */
int archive_read(
    const char *archive_path,
    ArchiveEntry **entries,
    size_t *entry_count
);


/* Release memory allocated for archive entries */
void archive_free_entries(
    ArchiveEntry *entries,
    size_t entry_count
);

#endif
