#include "file_io.h"

#include <stdio.h>
#include <string.h>

int main(){
    const char *filename = "test_data/file_io_test.bin";

    unsigned char original_data[] = {
        'H', 'e', 'l', 'l', 'o', 0, 255, 128, '!'
    };

    size_t original_size = sizeof(original_data);

    /* Step 1: Write the original data to a file. */
    if (write_file(filename, original_data, original_size) != 0){
        printf("TEST FAILED: Could not write file.\n");
        return 1;
    }

    printf("Step 1 passed: File written successfully.\n");

    /* Step 2: Read the file back into memory. */
    FileBuffer buffer = {NULL, 0};

    if (read_file(filename, &buffer) != 0){
        printf("TEST FAILED: Could not read file.\n");
        return 1;
    }

    printf("Step 2 passed: File read successfully.\n");

    /* Step 3: Compare the original and read data. */
    if (buffer.size != original_size ||memcmp(buffer.data, original_data, original_size) != 0){
        printf("TEST FAILED: Data does not match.\n");
        free_file_buffer(&buffer);
        return 1;
    }

    printf("Step 3 passed: Data matches exactly.\n");

    /* Step 4: Release allocated memory. */
    free_file_buffer(&buffer);

    printf("Step 4 passed: Memory released.\n");

    printf("\nALL FILE I/O TESTS PASSED!\n");

    return 0;
}