#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>

/*
 * Calculate the IEEE CRC-32 checksum.
 *
 * Parameters:
 * data     - Input byte array.
 * size     - Number of bytes.
 * checksum - Stores the calculated CRC32 value.
 *
 * Returns:
 *  0  on success.
 * -1  on invalid arguments.
 */

int crc32_calculate(
    const uint8_t *data,
    size_t size,
    uint32_t *checksum
);

#endif