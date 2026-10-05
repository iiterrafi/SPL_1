#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h>

typedef struct{
    unsigned char *data;
    size_t size;
} FileBuffer;

int read_file(const char *filename, FileBuffer *buffer);

int write_file(const char *filename, const unsigned char *data, size_t size);

void free_file_buffer(FileBuffer *buffer);

#endif
