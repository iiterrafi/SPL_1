#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

#include "file_compression.h"
#include "file_io.h"
#include "compressor.h"
#include "archive.h"
#include "crc32.h"

/* Compress one file into an archive. */
int compress_file_to_archive(
    const char *input_path,
    const char *archive_path,
    size_t max_chain_length){

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
/* Compress multiple files into one archive. */
int compress_files_to_archive(
    const char *const input_paths[],
    size_t file_count,
    const char *archive_path,
    size_t max_chain_length){

    if (!input_paths || file_count == 0 ||
        !archive_path || max_chain_length == 0 ||
        file_count > UINT32_MAX)
        return -1;

    ArchiveEntry *entries = calloc(file_count, sizeof(ArchiveEntry));
    FileBuffer *originals = calloc(file_count, sizeof(FileBuffer));
    uint8_t **compressed_buffers = calloc(
        file_count, sizeof(uint8_t *));

    if (!entries || !originals || !compressed_buffers)
        goto failure;

    for (size_t i = 0; i < file_count; i++)
    {
        if (!input_paths[i] || input_paths[i][0] == '\0')
            goto failure;

        /* Prevent accidentally overwriting an input file. */
        if (strcmp(input_paths[i], archive_path) == 0)
            goto failure;

        if (read_file(input_paths[i], &originals[i]) != 0)
            goto failure;

        size_t compressed_size = 0;
        CompressorMetadata metadata = {0};

        if (compressor_compress(
                originals[i].data,
                originals[i].size,
                &compressed_buffers[i],
                &compressed_size,
                &metadata,
                max_chain_length) != 0)
            goto failure;

        uint32_t checksum;

        if (crc32_calculate(
                originals[i].data,
                originals[i].size,
                &checksum) != 0)
            goto failure;

        entries[i].path = (char *)input_paths[i];
        entries[i].type = ARCHIVE_ENTRY_FILE;
        entries[i].original_size = originals[i].size;
        entries[i].crc32 = checksum;

        if (compressed_size < originals[i].size)
        {
            entries[i].method = ARCHIVE_METHOD_HYBRID;
            entries[i].payload = compressed_buffers[i];
            entries[i].payload_size = compressed_size;
            entries[i].metadata = metadata;
        }
        else
        {
            entries[i].method = ARCHIVE_METHOD_RAW;
            entries[i].payload = originals[i].data;
            entries[i].payload_size = originals[i].size;
        }
    }

    if (archive_write(archive_path, entries, file_count) != 0)
        goto failure;

    printf("Multi-file archive created successfully.\n");
    printf("Number of files: %zu\n", file_count);

    for (size_t i = 0; i < file_count; i++)
    {
        printf("%s: %zu bytes -> %zu bytes (%s)\n",
               entries[i].path,
               (size_t)entries[i].original_size,
               entries[i].payload_size,
               entries[i].method == ARCHIVE_METHOD_HYBRID
                   ? "HYBRID" : "RAW");
    }

    for (size_t i = 0; i < file_count; i++)
    {
        free(compressed_buffers[i]);
        free_file_buffer(&originals[i]);
    }

    free(compressed_buffers);
    free(originals);
    free(entries);

    return 0;

failure:
    if (compressed_buffers && originals)
    {
        for (size_t i = 0; i < file_count; i++)
        {
            free(compressed_buffers[i]);
            free_file_buffer(&originals[i]);
        }
    }

    free(compressed_buffers);
    free(originals);
    free(entries);

    fprintf(stderr, "Multi-file compression failed.\n");

    return -1;
}

/* Validate a relative archive path before extraction. */
static int safe_archive_path(const char *path)
{
    if (!path || !path[0] || path[0] == '/' || path[0] == '\\')
        return 0;

    const char *component = path;

    for (const char *p = path; ; p++)
    {
        if (*p == '\\' || *p == ':')
            return 0;

        if (*p == '/' || *p == '\0')
        {
            size_t length = (size_t)(p - component);

            if (length == 0 ||
                (length == 1 && component[0] == '.') ||
                (length == 2 && component[0] == '.' &&
                 component[1] == '.'))
                return 0;

            if (*p == '\0')
                break;

            component = p + 1;
        }
    }

    return 1;
}

/* Open or create one child directory without following symlinks. */
static int open_child_directory(int parent_fd, const char *name)
{
    if (mkdirat(parent_fd, name, 0755) != 0 && errno != EEXIST)
        return -1;

    return openat(
        parent_fd,
        name,
        O_RDONLY | O_DIRECTORY | O_NOFOLLOW
    );
}

/* Create and open a nested directory relative to root_fd. */
static int open_directory_chain(int root_fd, const char *path)
{
    char *copy = strdup(path);

    if (!copy)
        return -1;

    int current_fd = dup(root_fd);

    if (current_fd < 0)
    {
        free(copy);
        return -1;
    }

    char *saveptr = NULL;
    char *component = strtok_r(copy, "/", &saveptr);

    while (component)
    {
        int next_fd = open_child_directory(current_fd, component);

        if (next_fd < 0)
        {
            close(current_fd);
            free(copy);
            return -1;
        }

        close(current_fd);
        current_fd = next_fd;

        component = strtok_r(NULL, "/", &saveptr);
    }

    free(copy);
    return current_fd;
}

/* Write a file without overwriting an existing file. */
static int write_extracted_file(
    int parent_fd,
    const char *filename,
    const uint8_t *data,
    size_t size)
{
    int fd = openat(
        parent_fd,
        filename,
        O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW,
        0600
    );

    if (fd < 0)
        return -1;

    size_t written = 0;
    int status = 0;

    while (written < size)
    {
        size_t remaining = size - written;
        size_t chunk = remaining;

        if (chunk > (size_t)SSIZE_MAX)
            chunk = (size_t)SSIZE_MAX;

        ssize_t result = write(fd, data + written, chunk);

        if (result < 0)
        {
            if (errno == EINTR)
                continue;

            status = -1;
            break;
        }

        if (result == 0)
        {
            status = -1;
            break;
        }

        written += (size_t)result;
    }

    if (close(fd) != 0)
        status = -1;

    if (status != 0)
        unlinkat(parent_fd, filename, 0);

    return status;
}

/* Restore and verify one archive entry. */
static int restore_archive_entry(
    int root_fd,
    const ArchiveEntry *entry)
{
    if (!safe_archive_path(entry->path))
        return -1;

    /* Restore directory entries. */
    if (entry->type == ARCHIVE_ENTRY_DIRECTORY)
    {
        int directory_fd = open_directory_chain(root_fd, entry->path);

        if (directory_fd < 0)
            return -1;

        close(directory_fd);
        return 0;
    }

    if (entry->type != ARCHIVE_ENTRY_FILE ||
        entry->original_size > SIZE_MAX)
        return -1;

    uint8_t *restored = NULL;
    const uint8_t *data = NULL;
    size_t restored_size = 0;
    int allocated = 0;

    /* RAW: use the original bytes stored in the archive. */
    if (entry->method == ARCHIVE_METHOD_RAW)
    {
        if (entry->payload_size != entry->original_size)
            return -1;

        data = entry->payload;
        restored_size = entry->payload_size;
    }
    /* HYBRID: run the complete decompression pipeline. */
    else if (entry->method == ARCHIVE_METHOD_HYBRID)
    {
        if (compressor_decompress(
                entry->payload,
                entry->payload_size,
                &entry->metadata,
                &restored,
                &restored_size) != 0)
            return -1;

        data = restored;
        allocated = 1;
    }
    else
    {
        return -1;
    }

    /* Verify original size. */
    if (restored_size != entry->original_size)
        goto failure;

    /* Verify CRC32 before writing. */
    uint32_t checksum;

    if (crc32_calculate(data, restored_size, &checksum) != 0 ||
        checksum != entry->crc32)
        goto failure;

    /* Separate parent directory and filename. */
    char *path_copy = strdup(entry->path);

    if (!path_copy)
        goto failure;

    char *slash = strrchr(path_copy, '/');
    char *filename = path_copy;
    int parent_fd;

    if (slash)
    {
        *slash = '\0';
        filename = slash + 1;

        parent_fd = open_directory_chain(root_fd, path_copy);
    }
    else
    {
        parent_fd = dup(root_fd);
    }

    if (parent_fd < 0)
    {
        free(path_copy);
        goto failure;
    }

    int write_status = write_extracted_file(
        parent_fd,
        filename,
        data,
        restored_size
    );

    close(parent_fd);
    free(path_copy);

    if (write_status != 0)
        goto failure;

    if (allocated)
        free(restored);

    printf("Extracted: %s (%zu bytes)\n",
           entry->path, restored_size);

    return 0;

failure:
    if (allocated)
        free(restored);

    return -1;
}

/* Extract all files and directories from a .cmp archive. */
int decompress_archive_to_directory(
    const char *archive_path,
    const char *output_directory)
{
    if (!archive_path || !output_directory ||
        !output_directory[0])
        return -1;

    ArchiveEntry *entries = NULL;
    size_t entry_count = 0;

    int root_fd = -1;
    int status = -1;

    if (archive_read(
            archive_path,
            &entries,
            &entry_count) != 0)
        goto cleanup;

    /* Validate every path before extracting anything. */
    for (size_t i = 0; i < entry_count; i++)
    {
        if (!safe_archive_path(entries[i].path))
            goto cleanup;

        /* Reject duplicate archive paths. */
        for (size_t j = 0; j < i; j++)
        {
            if (strcmp(entries[i].path, entries[j].path) == 0)
                goto cleanup;
        }
    }

    /* Create the destination directory if necessary. */
    struct stat st;

    if (lstat(output_directory, &st) != 0)
    {
        if (errno != ENOENT ||
            mkdir(output_directory, 0755) != 0)
            goto cleanup;

        if (lstat(output_directory, &st) != 0)
            goto cleanup;
    }

    /* Reject symlinks and non-directory destinations. */
    if (!S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode))
        goto cleanup;

    root_fd = open(
        output_directory,
        O_RDONLY | O_DIRECTORY | O_NOFOLLOW
    );

    if (root_fd < 0)
        goto cleanup;

    printf("\nStarting archive extraction...\n");
    printf("Archive entries: %zu\n\n", entry_count);

    for (size_t i = 0; i < entry_count; i++)
    {
        if (restore_archive_entry(root_fd, &entries[i]) != 0)
        {
            fprintf(stderr,
                    "Extraction failed: %s\n",
                    entries[i].path);

            goto cleanup;
        }
    }

    printf("\nExtraction completed successfully.\n");
    printf("Total entries extracted: %zu\n", entry_count);

    status = 0;

cleanup:
    if (root_fd >= 0)
        close(root_fd);

    archive_free_entries(entries, entry_count);

    if (status != 0)
        fprintf(stderr, "Archive extraction failed.\n");

    return status;
}

/* One collected item used while building a folder archive. */
typedef struct
{
    ArchiveEntry entry;
    FileBuffer original;
    uint8_t *compressed;
} FolderItem;

typedef struct
{
    FolderItem *items;
    size_t count;
    size_t capacity;
    size_t max_chain_length;
} FolderBuild;

/* Join two filesystem or archive paths safely. */
static char *folder_join_path(
    const char *parent,
    const char *name)
{
    size_t parent_length = strlen(parent);
    size_t name_length = strlen(name);

    if (parent_length > SIZE_MAX - name_length - 2)
        return NULL;

    char *result = malloc(parent_length + name_length + 2);

    if (!result)
        return NULL;

    snprintf(result,
             parent_length + name_length + 2,
             "%s/%s",
             parent,
             name);

    return result;
}

/* Reserve space for another collected archive entry. */
static FolderItem *folder_new_item(FolderBuild *build)
{
    if (build->count >= UINT32_MAX)
        return NULL;

    if (build->count == build->capacity)
    {
        size_t new_capacity =
            build->capacity == 0 ? 16 : build->capacity * 2;

        if (new_capacity < build->capacity ||
            new_capacity > SIZE_MAX / sizeof(FolderItem))
            return NULL;

        FolderItem *new_items = realloc(
            build->items,
            new_capacity * sizeof(FolderItem)
        );

        if (!new_items)
            return NULL;

        build->items = new_items;
        build->capacity = new_capacity;
    }

    FolderItem *item = &build->items[build->count];
    memset(item, 0, sizeof(*item));

    return item;
}

/* Recursively collect directories and compress regular files. */
static int collect_folder_entries(
    FolderBuild *build,
    const char *full_path,
    const char *relative_path)
{
    struct stat st;

    if (lstat(full_path, &st) != 0)
        return -1;

    if (S_ISLNK(st.st_mode))
    {
        fprintf(stderr, "Symbolic links are not supported: %s\n",
                full_path);
        return -1;
    }

    if (strlen(relative_path) > UINT16_MAX)
        return -1;

    /* Add a directory entry, including empty directories. */
    if (S_ISDIR(st.st_mode))
    {
        FolderItem *item = folder_new_item(build);

        if (!item)
            return -1;

        build->count++;

        item->entry.path = strdup(relative_path);

        if (!item->entry.path)
            return -1;

        item->entry.type = ARCHIVE_ENTRY_DIRECTORY;
        item->entry.method = ARCHIVE_METHOD_RAW;
        item->entry.original_size = 0;
        item->entry.crc32 = 0;
        item->entry.payload = NULL;
        item->entry.payload_size = 0;

        DIR *directory = opendir(full_path);

        if (!directory)
            return -1;

        int status = 0;

        for (;;)
        {
            errno = 0;
            struct dirent *de = readdir(directory);

            if (!de)
            {
                if (errno != 0)
                    status = -1;

                break;
            }

            if (strcmp(de->d_name, ".") == 0 ||
                strcmp(de->d_name, "..") == 0)
                continue;

            char *child_full = folder_join_path(
                full_path, de->d_name);

            char *child_relative = folder_join_path(
                relative_path, de->d_name);

            if (!child_full || !child_relative)
            {
                free(child_full);
                free(child_relative);
                status = -1;
                break;
            }

            if (collect_folder_entries(
                    build,
                    child_full,
                    child_relative) != 0)
                status = -1;

            free(child_full);
            free(child_relative);

            if (status != 0)
                break;
        }

        if (closedir(directory) != 0)
            status = -1;

        return status;
    }

    /* Only regular files are supported. */
    if (!S_ISREG(st.st_mode))
    {
        fprintf(stderr, "Unsupported file type: %s\n", full_path);
        return -1;
    }

    FolderItem *item = folder_new_item(build);

    if (!item)
        return -1;

    build->count++;

    item->entry.path = strdup(relative_path);

    if (!item->entry.path)
        return -1;

    if (read_file(full_path, &item->original) != 0)
        return -1;

    size_t compressed_size = 0;
    CompressorMetadata metadata = {0};

    if (compressor_compress(
            item->original.data,
            item->original.size,
            &item->compressed,
            &compressed_size,
            &metadata,
            build->max_chain_length) != 0)
        return -1;

    uint32_t checksum;

    if (crc32_calculate(
            item->original.data,
            item->original.size,
            &checksum) != 0)
        return -1;

    item->entry.type = ARCHIVE_ENTRY_FILE;
    item->entry.original_size = item->original.size;
    item->entry.crc32 = checksum;

    if (compressed_size < item->original.size)
    {
        item->entry.method = ARCHIVE_METHOD_HYBRID;
        item->entry.payload = item->compressed;
        item->entry.payload_size = compressed_size;
        item->entry.metadata = metadata;
    }
    else
    {
        item->entry.method = ARCHIVE_METHOD_RAW;
        item->entry.payload = item->original.data;
        item->entry.payload_size = item->original.size;
    }

    return 0;
}

/* Release all temporary folder-compression data. */
static void free_folder_build(FolderBuild *build)
{
    for (size_t i = 0; i < build->count; i++)
    {
        free(build->items[i].entry.path);
        free(build->items[i].compressed);
        free_file_buffer(&build->items[i].original);
    }

    free(build->items);

    build->items = NULL;
    build->count = 0;
    build->capacity = 0;
}

/* Compress an entire directory into one .cmp archive. */
int compress_directory_to_archive(
    const char *input_directory,
    const char *archive_path,
    size_t max_chain_length)
{
    if (!input_directory || !input_directory[0] ||
        !archive_path || !archive_path[0] ||
        max_chain_length == 0)
        return -1;

    struct stat root_stat;

    if (lstat(input_directory, &root_stat) != 0 ||
        !S_ISDIR(root_stat.st_mode) ||
        S_ISLNK(root_stat.st_mode))
        return -1;

    /* Determine the top-level folder name. */
    char *root_copy = strdup(input_directory);

    if (!root_copy)
        return -1;

    size_t root_length = strlen(root_copy);

    while (root_length > 1 &&
           root_copy[root_length - 1] == '/')
    {
        root_copy[--root_length] = '\0';
    }

    char *root_name = strrchr(root_copy, '/');

    root_name = root_name ? root_name + 1 : root_copy;

    if (!root_name[0] ||
        strcmp(root_name, ".") == 0 ||
        strcmp(root_name, "..") == 0)
    {
        free(root_copy);
        fprintf(stderr, "Please provide a named directory path.\n");
        return -1;
    }

    FolderBuild build = {0};
    build.max_chain_length = max_chain_length;

    int status = -1;

    printf("\nStarting folder compression...\n");
    printf("Source directory: %s\n\n", input_directory);

    if (collect_folder_entries(
            &build,
            input_directory,
            root_name) != 0)
        goto cleanup;

    if (build.count == 0 ||
        build.count > SIZE_MAX / sizeof(ArchiveEntry))
        goto cleanup;

    /* Create a contiguous array for archive_write(). */
    ArchiveEntry *entries = malloc(
        build.count * sizeof(ArchiveEntry));

    if (!entries)
        goto cleanup;

    for (size_t i = 0; i < build.count; i++)
        entries[i] = build.items[i].entry;

    if (archive_write(
            archive_path,
            entries,
            build.count) != 0)
    {
        free(entries);
        goto cleanup;
    }

    printf("\nFolder archive created successfully.\n");
    printf("Total archive entries: %zu\n", build.count);

    for (size_t i = 0; i < build.count; i++)
    {
        if (entries[i].type == ARCHIVE_ENTRY_DIRECTORY)
        {
            printf("[DIR]  %s\n", entries[i].path);
        }
        else
        {
            printf("[FILE] %s: %zu -> %zu bytes (%s)\n",
                   entries[i].path,
                   (size_t)entries[i].original_size,
                   entries[i].payload_size,
                   entries[i].method == ARCHIVE_METHOD_HYBRID
                       ? "HYBRID" : "RAW");
        }
    }

    free(entries);
    status = 0;

cleanup:
    free_folder_build(&build);
    free(root_copy);

    if (status != 0)
        fprintf(stderr, "Folder compression failed.\n");

    return status;
}
