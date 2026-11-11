#ifndef FILE_COMPRESSION_H
#define FILE_COMPRESSION_H

#include <stddef.h>

/* Compress one file into a .cmp archive. */
int compress_file_to_archive(
    const char *input_path,
    const char *archive_path,
    size_t max_chain_length
);

/* Compress multiple files into one .cmp archive. */
int compress_files_to_archive(
    const char *const input_paths[],
    size_t file_count,
    const char *archive_path,
    size_t max_chain_length
);

/* Compress an entire directory, including subdirectories. */
int compress_directory_to_archive(
    const char *input_directory,
    const char *archive_path,
    size_t max_chain_length
);

/* Restore one file from a .cmp archive. */
int decompress_archive_to_file(
    const char *archive_path,
    const char *output_path
);

/* Restore all files and directories from a .cmp archive. */
int decompress_archive_to_directory(
    const char *archive_path,
    const char *output_directory
);

#endif