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

    if (byte != 0xB0){
        printf("FAIL: Expected byte 0xB0, got 0x%02X.\n",
               (unsigned int)(unsigned char)byte);
        fclose(file);
        return 1;
    }

    printf("Step 2 passed: Bits packed into the correct byte.\n");

    /* Read back the five meaningful bits */
    rewind(file);

    BitReader reader;

    if (bit_reader_init(&reader, file) != 0){
        printf("FAIL: Bit reader initialization.\n");
        fclose(file);
        return 1;
    }

    uint32_t value = 0;

    if(bit_read_bits(&reader, 5, &value) != 0){
        printf("FAIL: Reading bits.\n");
        fclose(file);
        return 1;
    }

    if(value != 22){
        printf("FAIL: Expected 10110, got %u.\n",
               (unsigned int)value);
        fclose(file);
        return 1;
    }

    printf("Step 3 passed: Bits read back correctly.\n");

        /* Test writing and reading a complete byte */
    FILE *byte_file = tmpfile();

    if(byte_file == NULL){
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
        /* Test writing and reading bits across a byte boundary */
    FILE *multi_file = tmpfile();

    if (multi_file == NULL) {
        printf("FAIL: Could not create multi-byte test file.\n");
        fclose(file);
        return 1;
    }

    BitWriter multi_writer;

    if (bit_writer_init(&multi_writer, multi_file) != 0 || bit_write_bits(&multi_writer, 0xB2D, 12) != 0 || bit_writer_flush(&multi_writer) != 0) {
        printf("FAIL: Writing 12 bits.\n");
        fclose(multi_file);
        fclose(file);
        return 1;
    }

    /* Check the two bytes produced */
    rewind(multi_file);

    int first_byte = fgetc(multi_file);
    int second_byte = fgetc(multi_file);

    if (first_byte != 0xB2 || second_byte != 0xD0) {
        printf("FAIL: Incorrect bytes produced.\n");
        fclose(multi_file);
        fclose(file);
        return 1;
    }

    /* Read the original 12 bits back */
    rewind(multi_file);

    BitReader multi_reader;
    uint32_t multi_result = 0;

    if (bit_reader_init(&multi_reader, multi_file) != 0 || bit_read_bits(&multi_reader, 12, &multi_result) != 0 || multi_result != 0xB2D) {
        printf("FAIL: Reading 12 bits across a byte boundary.\n");
        fclose(multi_file);
        fclose(file);
        return 1;
    }

    fclose(multi_file);

    printf("Step 5 passed: Bits correctly handled across byte boundaries.\n");

    

        /* Test invalid inputs */
    if (bit_write_bit(&writer, 2) != -1) {
        printf("FAIL: Invalid bit value was accepted.\n");
        fclose(file);
        return 1;
    }

    if(bit_write_bits(&writer, 0, 33) != -1){
        printf("FAIL: Invalid bit count was accepted by writer.\n");
        fclose(file);
        return 1;
    }

    if(bit_read_bits(&reader, 33, &value) != -1){
        printf("FAIL: Invalid bit count was accepted by reader.\n");
        fclose(file);
        return 1;
    }

    printf("Step 6 passed: Invalid inputs handled correctly.\n");

    fclose(file);
    
    printf("\nALL BITSTREAM TESTS PASSED!\n");

    return 0;
}