#ifndef FILE_COMPRESSION_H
#define FILE_COMPRESSION_H

#include <stddef.h>

/* Compress one file into a .cmp archive. */
int compress_file_to_archive(
    const char *input_path,
    const char *archive_path,
    size_t max_chain_length
);

/* Restore one file from a .cmp archive. */
int decompress_archive_to_file(
    const char *archive_path,
    const char *output_path
);

#endif
