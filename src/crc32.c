#include "crc32.h"

/*
 * Standard IEEE CRC-32 polynomial.
 */
#define CRC32_POLYNOMIAL 0xEDB88320U

int crc32_calculate(
    const uint8_t *data,
    size_t size,
    uint32_t *checksum
) {
    /* Validate arguments. */
    if (checksum == NULL) {
        return -1;
    }

    if (data == NULL && size > 0) {
        return -1;
    }

    /* Initial CRC value. */
    uint32_t crc = 0xFFFFFFFFU;

    /* Process every byte. */
    for (size_t i = 0; i < size; i++) {

        crc ^= data[i];

        /* Process all 8 bits of the current byte. */
        for (int j = 0; j < 8; j++) {

            if (crc & 1U) {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
    }

    /* Final XOR operation. */
    crc ^= 0xFFFFFFFFU;

    *checksum = crc;

    return 0;
}