#include "bitstream.h"

#include <stdio.h>
#include <stdint.h>

int main(void)
{
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

    fclose(file);

    printf("\nALL BITSTREAM TESTS PASSED!\n");

    return 0;
}