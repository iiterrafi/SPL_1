#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

int read_file(const char *filename, FileBuffer *buffer){
    if (filename == NULL || buffer == NULL){
        return -1;
    }

    buffer->data = NULL;
    buffer->size = 0;

    FILE *file = fopen(filename, "rb");
    if(file == NULL){
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0){
        fclose(file);
        return -1;
    }

    long file_size = ftell(file);

    if(file_size < 0){
        fclose(file);
        return -1;
    }
    if (fseek(file, 0, SEEK_SET) != 0){
        fclose(file);
        return -1;
    }
    if (file_size == 0){
        if (fclose(file) != 0){
            return -1;
        }
        return 0;
    }

    buffer->data = malloc((size_t)file_size);
    if(buffer->data == NULL){
        fclose(file);
        return -1;
    }

    size_t bytes_read = fread(buffer->data, 1, (size_t)file_size, file);
    if(bytes_read != (size_t)file_size){
        free(buffer->data);
        buffer->data = NULL;
        buffer->size = 0;
        fclose(file);
        return -1;
    }

    buffer->size = (size_t)file_size;

    if (fclose(file) != 0){
        free(buffer->data);
        buffer->data = NULL;
        buffer->size = 0;
        return -1;
    }

    return 0;
}




int write_file(const char *filename, const unsigned char *data, size_t size){
    if (filename == NULL || (data == NULL && size > 0)){
        return -1;
    }

    FILE *file = fopen(filename, "wb");

    if (file == NULL){
        return -1;
    }

    if (size > 0){
        size_t bytes_written = fwrite(data, 1, size, file);

        if (bytes_written != size){
            fclose(file);
            return -1;
        }
    }

    if (fclose(file) != 0){
        return -1;
    }

    return 0;
}





void free_file_buffer(FileBuffer *buffer){
    if (buffer == NULL){
        return;
    }

    free(buffer->data);

    buffer->data = NULL;
    buffer->size = 0;
}