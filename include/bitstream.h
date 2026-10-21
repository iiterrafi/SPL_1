#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/* Bit writer structure */
typedef struct {
    FILE *file;
    uint8_t buffer;
    unsigned int bit_count;
} BitWriter;

/* Bit reader structure */
typedef struct {
    FILE *file;
    uint8_t buffer;
    unsigned int bit_count;
} BitReader;

/* Bit writer functions */
int bit_writer_init(BitWriter *writer, FILE *file);
int bit_write_bit(BitWriter *writer, unsigned int bit);
int bit_write_bits(BitWriter *writer, uint32_t value, unsigned int count);
int bit_writer_flush(BitWriter *writer);

/* Bit reader functions */
int bit_reader_init(BitReader *reader, FILE *file);
int bit_read_bit(BitReader *reader, unsigned int *bit);
int bit_read_bits(BitReader *reader, unsigned int count, uint32_t *value);

#endif