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

    /* Step 5: Test writing an empty file. */
    const char *empty_filename = "test_data/empty_test.txt";

    if (write_file(empty_filename, NULL, 0) != 0){
        printf("TEST FAILED: Could not write empty file.\n");
        return 1;
    }

    printf("Step 5 passed: Empty file written successfully.\n");

    /* Step 6: Test reading the empty file. */
    FileBuffer empty_buffer = {NULL, 0};

    if (read_file(empty_filename, &empty_buffer) != 0){
        printf("TEST FAILED: Could not read empty file.\n");
        return 1;
    }

    if (empty_buffer.data != NULL || empty_buffer.size != 0){
        printf("TEST FAILED: Empty file buffer is incorrect.\n");
        free_file_buffer(&empty_buffer);
        return 1;
    }

    printf("Step 6 passed: Empty file read successfully.\n");

    free_file_buffer(&empty_buffer);

        /* Step 7: Test invalid arguments. */

    if (write_file(NULL, original_data, original_size) != -1){
        printf("TEST FAILED: write_file accepted a NULL filename.\n");
        return 1;
    }

    if (write_file("test_data/invalid_test.bin", NULL, 1) != -1){
        printf("TEST FAILED: write_file accepted NULL data with nonzero size.\n");
        return 1;
    }

    if (read_file(NULL, &buffer) != -1){
        printf("TEST FAILED: read_file accepted a NULL filename.\n");
        return 1;
    }

    if (read_file(filename, NULL) != -1){
        printf("TEST FAILED: read_file accepted a NULL buffer.\n");
        return 1;
    }

    printf("Step 7 passed: Invalid arguments rejected correctly.\n");

    printf("\nALL FILE I/O TESTS PASSED!\n");

    return 0;
}