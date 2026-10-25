#include "bitstream.h"

#include <stdio.h>
#include <stdint.h>

int main(){
    FILE *file = tmpfile();

    if (file == NULL) {
        printf("FAIL: Could not create temporary file.\n");
        return 1;
    }

    BitWriter writer;

    if (bit_writer_init(&writer, file) != 0) {
        printf("FAIL: Bit writer initialization.\n");
        fclose(file);
        return 1;
    }

    /* Write five bits: 10110 */
    if (bit_write_bits(&writer, 22, 5) != 0) {
        printf("FAIL: Writing bits.\n");
        fclose(file);
        return 1;
    }

    if (bit_writer_flush(&writer) != 0) {
        printf("FAIL: Flushing bit writer.\n");
        fclose(file);
        return 1;
    }

    printf("Step 1 passed: Bits written successfully.\n");

    /* Check the resulting byte */
    rewind(file);

    int byte = fgetc(file);

    if (byte != 0xB0) {
        printf("FAIL: Expected byte 0xB0, got 0x%02X.\n",
               (unsigned int)(unsigned char)byte);
        fclose(file);
        return 1;
    }

    printf("Step 2 passed: Bits packed into the correct byte.\n");

    /* Read back the five meaningful bits */
    rewind(file);

    BitReader reader;

    if (bit_reader_init(&reader, file) != 0) {
        printf("FAIL: Bit reader initialization.\n");
        fclose(file);
        return 1;
    }

    uint32_t value = 0;

    if (bit_read_bits(&reader, 5, &value) != 0) {
        printf("FAIL: Reading bits.\n");
        fclose(file);
        return 1;
    }

    if (value != 22) {
        printf("FAIL: Expected 10110, got %u.\n",
               (unsigned int)value);
        fclose(file);
        return 1;
    }

    printf("Step 3 passed: Bits read back correctly.\n");

        /* Test writing and reading a complete byte */
    FILE *byte_file = tmpfile();

    if (byte_file == NULL) {
        printf("FAIL: Could not create byte test file.\n");
        fclose(file);
        return 1;
    }

    BitWriter byte_writer;

    if (bit_writer_init(&byte_writer, byte_file) != 0 || bit_write_bits(&byte_writer, 165, 8) != 0) {
        printf("FAIL: Writing a complete byte.\n");
        fclose(byte_file);
        fclose(file);
        return 1;
    }

    rewind(byte_file);

    int byte_value = fgetc(byte_file);

    if (byte_value != 165) {
        printf("FAIL: Expected 165, got %d.\n", byte_value);
        fclose(byte_file);
        fclose(file);
        return 1;
    }

    rewind(byte_file);

    BitReader byte_reader;
    uint32_t byte_result = 0;

    if (bit_reader_init(&byte_reader, byte_file) != 0 || bit_read_bits(&byte_reader, 8, &byte_result) != 0 ||byte_result != 165) {
        printf("FAIL: Reading a complete byte.\n");
        fclose(byte_file);
        fclose(file);
        return 1;
    }

    fclose(byte_file);

    printf("Step 4 passed: Complete byte written and read correctly.\n");

    fclose(file);

    printf("\nALL BITSTREAM TESTS PASSED!\n");

    return 0;
}