#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "crc32.h"

int main(void) {

    uint32_t checksum;

    printf("Testing CRC32...\n\n");

    /* Test 1: Standard CRC32 test vector. */

    const uint8_t text[] = "123456789";

    if (crc32_calculate(text, 9, &checksum) != 0)
        return 1;

    if (checksum != 0xCBF43926U) {
        printf("FAIL: Standard test vector.\n");
        printf("Expected: 0xCBF43926\n");
        printf("Received: 0x%08X\n", checksum);
        return 1;
    }

    printf("PASS: Standard CRC32 test vector.\n");
    printf("      CRC32 = 0x%08X\n", checksum);


    /* Test 2: Empty input. */

    if (crc32_calculate(NULL, 0, &checksum) != 0)
        return 1;

    if (checksum != 0x00000000U)
        return 1;

    printf("PASS: Empty input.\n");


    /* Test 3: Binary data. */

    const uint8_t binary_data[] = {
        0x00, 0xFF, 0x80, 0x01,
        0xAA, 0x55, 0x00, 0xFE
    };

    if (crc32_calculate(
            binary_data,
            sizeof(binary_data),
            &checksum
        ) != 0)
        return 1;

    printf("PASS: Binary data.\n");
    printf("      CRC32 = 0x%08X\n", checksum);


    /* Test 4: Single-byte input. */

    const uint8_t single_byte[] = {0x41};

    if (crc32_calculate(
            single_byte,
            sizeof(single_byte),
            &checksum
        ) != 0)
        return 1;

    printf("PASS: Single-byte input.\n");


    /* Test 5: Invalid arguments. */

    if (crc32_calculate(NULL, 5, &checksum) != -1)
        return 1;

    printf("PASS: NULL input rejected.\n");

    if (crc32_calculate(text, 9, NULL) != -1)
        return 1;

    printf("PASS: NULL checksum pointer rejected.\n");


    

        /* Test 6: Detect modified data. */

    uint8_t original[] = "Hello World";
    uint8_t modified[] = "Hello Worle";

    uint32_t original_crc;
    uint32_t modified_crc;

    if (crc32_calculate(
            original, sizeof(original) - 1, &original_crc
        ) != 0)
        return 1;

    if (crc32_calculate(
            modified, sizeof(modified) - 1, &modified_crc
        ) != 0)
        return 1;

    if (original_crc == modified_crc) {
        printf("FAIL: Modified data was not detected.\n");
        return 1;
    }

    printf("PASS: Modified data corruption detected.\n");
    printf("\nALL CRC32 TESTS PASSED!\n");
    
    return 0;
}