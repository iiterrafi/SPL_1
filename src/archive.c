#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "archive.h"

/* ================= INTEGER WRITING ================= */

static int allocation_size_valid(uint32_t count)
{
    size_t n = (size_t)count;

    return n == 0 || sizeof(ArchiveEntry) <= SIZE_MAX / n;
}

static int write_u16(FILE *f, uint16_t v)
{
    for (int i = 0; i < 2; i++)
        if (fputc((v >> (i * 8)) & 0xFF, f) == EOF)
            return -1;
    return 0;
}

static int write_u32(FILE *f, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        if (fputc((v >> (i * 8)) & 0xFF, f) == EOF)
            return -1;
    return 0;
}

static int write_u64(FILE *f, uint64_t v)
{
    for (int i = 0; i < 8; i++)
        if (fputc((v >> (i * 8)) & 0xFF, f) == EOF)
            return -1;
    return 0;
}

/* ================= INTEGER READING ================= */

static int read_u16(FILE *f, uint16_t *v){
    uint16_t result = 0;

    for (int i = 0; i < 2; i++)
    {
        int b = fgetc(f);
        if (b == EOF) return -1;
        result |= (uint16_t)((uint16_t)b << (i * 8));
    }

    *v = result;
    return 0;
}

static int read_u32(FILE *f, uint32_t *v)
{
    uint32_t result = 0;

    for (int i = 0; i < 4; i++)
    {
        int b = fgetc(f);
        if (b == EOF) return -1;
        result |= (uint32_t)b << (i * 8);
    }

    *v = result;
    return 0;
}

static int read_u64(FILE *f, uint64_t *v)
{
    uint64_t result = 0;

    for (int i = 0; i < 8; i++)
    {
        int b = fgetc(f);
        if (b == EOF) return -1;
        result |= (uint64_t)b << (i * 8);
    }

    *v = result;
    return 0;
}

/* ================= COMPRESSION METADATA ================= */

static int write_metadata(FILE *f, const CompressorMetadata *m)
{
    if (write_u64(f, m->original_size) != 0 ||
        write_u64(f, m->rle_size) != 0 ||
        write_u64(f, m->token_count) != 0 ||
        write_u64(f, m->serialized_size) != 0 ||
        write_u64(f, m->meaningful_bits) != 0 ||
        write_u32(f, m->crc32) != 0)
        return -1;

    for (int i = 0; i < 256; i++)
        if (write_u16(f, m->huffman_code_lengths[i]) != 0)
            return -1;

    return 0;
}

static int read_metadata(FILE *f, CompressorMetadata *m)
{
    if (read_u64(f, &m->original_size) != 0 ||
        read_u64(f, &m->rle_size) != 0 ||
        read_u64(f, &m->token_count) != 0 ||
        read_u64(f, &m->serialized_size) != 0 ||
        read_u64(f, &m->meaningful_bits) != 0 ||
        read_u32(f, &m->crc32) != 0)
        return -1;

    for (int i = 0; i < 256; i++)
        if (read_u16(f, &m->huffman_code_lengths[i]) != 0)
            return -1;

    return 0;
}

/* ================= ARCHIVE WRITING ================= */

int archive_write(
    const char *archive_path,
    const ArchiveEntry *entries,
    size_t entry_count)
{
    if (!archive_path ||
        (entry_count > 0 && !entries) ||
        entry_count > UINT32_MAX)
        return -1;

    FILE *f = fopen(archive_path, "wb");
    if (!f) return -1;

    int status = -1;

    /* Archive header */
    if (fwrite(ARCHIVE_MAGIC, 1, 4, f) != 4 ||
        write_u16(f, ARCHIVE_VERSION) != 0 ||
        write_u32(f, (uint32_t)entry_count) != 0)
        goto cleanup;

    for (size_t i = 0; i < entry_count; i++)
    {
        const ArchiveEntry *e = &entries[i];

        if (!e->path || e->type < ARCHIVE_ENTRY_FILE ||
            e->type > ARCHIVE_ENTRY_DIRECTORY ||
            (e->method != ARCHIVE_METHOD_RAW &&
             e->method != ARCHIVE_METHOD_HYBRID))
            goto cleanup;

        size_t path_len = strlen(e->path);

        if (path_len == 0 || path_len > UINT16_MAX ||
            (e->payload_size > 0 && !e->payload))
            goto cleanup;

        if (e->type == ARCHIVE_ENTRY_DIRECTORY &&
            (e->payload_size != 0 ||
             e->method != ARCHIVE_METHOD_RAW))
            goto cleanup;

        if (e->method == ARCHIVE_METHOD_HYBRID &&
            e->type != ARCHIVE_ENTRY_FILE)
            goto cleanup;

        /* Entry header */
        if (fputc(e->type, f) == EOF ||
            fputc(e->method, f) == EOF ||
            write_u16(f, (uint16_t)path_len) != 0 ||
            write_u64(f, e->original_size) != 0 ||
            write_u64(f, (uint64_t)e->payload_size) != 0 ||
            write_u32(f, e->crc32) != 0)
            goto cleanup;

        /* Hybrid metadata */
        if (e->method == ARCHIVE_METHOD_HYBRID)
        {
            if (e->metadata.original_size != e->original_size ||
                e->metadata.crc32 != e->crc32 ||
                write_metadata(f, &e->metadata) != 0)
                goto cleanup;
        }

        /* Path and payload */
        if (fwrite(e->path, 1, path_len, f) != path_len)
            goto cleanup;

        if (e->payload_size > 0 &&
            fwrite(e->payload, 1, e->payload_size, f) != e->payload_size)
            goto cleanup;
    }

    if (fflush(f) == 0 && !ferror(f))
        status = 0;

cleanup:
    if (fclose(f) != 0)
        status = -1;

    return status;
}

/* ================= ARCHIVE READING ================= */

void archive_free_entries(ArchiveEntry *entries, size_t count)
{
    if (!entries) return;

    for (size_t i = 0; i < count; i++)
    {
        free(entries[i].path);
        free(entries[i].payload);
    }

    free(entries);
}

int archive_read(
    const char *archive_path,
    ArchiveEntry **entries,
    size_t *entry_count)
{
    if (!archive_path || !entries || !entry_count)
        return -1;

    *entries = NULL;
    *entry_count = 0;

    FILE *f = fopen(archive_path, "rb");
    if (!f) return -1;

    int status = -1;
    ArchiveEntry *result = NULL;
    uint32_t count = 0;

    char magic[4];
    uint16_t version;

    if (fread(magic, 1, 4, f) != 4 ||
        memcmp(magic, ARCHIVE_MAGIC, 4) != 0 ||
        read_u16(f, &version) != 0 ||
        version != ARCHIVE_VERSION ||
        read_u32(f, &count) != 0)
        goto cleanup;

    if (!allocation_size_valid(count))
        goto cleanup;

    if (count > 0)
    {
        result = calloc(count, sizeof(ArchiveEntry));
        if (!result) goto cleanup;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        ArchiveEntry *e = &result[i];

        int type = fgetc(f);
        int method = fgetc(f);

        uint16_t path_len;
        uint64_t original_size;
        uint64_t payload_size;

        if (type == EOF || method == EOF ||
            read_u16(f, &path_len) != 0 ||
            read_u64(f, &original_size) != 0 ||
            read_u64(f, &payload_size) != 0 ||
            read_u32(f, &e->crc32) != 0)
            goto cleanup;

        e->type = (uint8_t)type;
        e->method = (uint8_t)method;
        e->original_size = original_size;

        if (path_len == 0 ||
            (e->type != ARCHIVE_ENTRY_FILE &&
             e->type != ARCHIVE_ENTRY_DIRECTORY) ||
            (e->method != ARCHIVE_METHOD_RAW &&
             e->method != ARCHIVE_METHOD_HYBRID) ||
            (e->type == ARCHIVE_ENTRY_DIRECTORY &&
             (payload_size != 0 ||
              e->method != ARCHIVE_METHOD_RAW)) ||
            (e->method == ARCHIVE_METHOD_HYBRID &&
             e->type != ARCHIVE_ENTRY_FILE) ||
            payload_size > SIZE_MAX)
            goto cleanup;

        if (e->method == ARCHIVE_METHOD_HYBRID)
        {
            if (read_metadata(f, &e->metadata) != 0 ||
                e->metadata.original_size != e->original_size ||
                e->metadata.crc32 != e->crc32)
                goto cleanup;
        }

        e->path = malloc((size_t)path_len + 1);
        if (!e->path) goto cleanup;

        if (fread(e->path, 1, path_len, f) != path_len)
            goto cleanup;

        e->path[path_len] = '\0';

        if (memchr(e->path, '\0', path_len) != NULL)
            goto cleanup;

        e->payload_size = (size_t)payload_size;

        if (e->payload_size > 0)
        {
            e->payload = malloc(e->payload_size);
            if (!e->payload) goto cleanup;

            if (fread(e->payload, 1, e->payload_size, f)
                != e->payload_size)
                goto cleanup;
        }
    }

    /* Reject unexpected trailing data */
    if (fgetc(f) != EOF || ferror(f))
        goto cleanup;

    *entries = result;
    *entry_count = count;
    result = NULL;
    status = 0;

cleanup:
    archive_free_entries(result, count);
    fclose(f);

    return status;
}