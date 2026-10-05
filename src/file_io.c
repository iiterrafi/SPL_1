#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

int read_file(const char *filename, FileBuffer *buffer){
    FILE *file = fopen(filename, "rb");
    if (file == NULL){
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);

    if (file_size < 0){
        fclose(file);
        return -1;
    }
    rewind(file);

    if (file_size == 0){
        buffer->data = NULL;
        buffer->size = 0;
        fclose(file);
        return 0;
    }

    buffer->data = malloc((size_t)file_size);
    if (buffer->data == NULL){
        fclose(file);
        return -1;
    }

    size_t bytes_read = fread(buffer->data, 1, (size_t)file_size, file);
    if (bytes_read != (size_t)file_size){
        free(buffer->data);
        fclose(file);
        return -1;
    }

    buffer->size = (size_t)file_size;
    fclose(file);
    
    return 0;
}