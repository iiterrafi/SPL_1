#include "bitstream.h"

/* Initialize the bit writer */
int bit_writer_init(BitWriter *writer, FILE *file){
    if (writer == NULL || file == NULL) {
        return -1;
    }

    writer->file = file;
    writer->buffer = 0;
    writer->bit_count = 0;

    return 0;
}

/* Write a single bit */
int bit_write_bit(BitWriter *writer, unsigned int bit){
    if (writer == NULL || writer->file == NULL || bit > 1) {
        return -1;
    }

    writer->buffer = (uint8_t)((writer->buffer << 1) | bit);
    writer->bit_count++;

    if (writer->bit_count == 8) {
        if (fputc(writer->buffer, writer->file) == EOF) {
            return -1;
        }

        writer->buffer = 0;
        writer->bit_count = 0;
    }

    return 0;
}

/* Write multiple bits, from most significant to least significant */
int bit_write_bits(BitWriter *writer, uint32_t value, unsigned int count){
    if (writer == NULL || writer->file == NULL || count > 32) {
        return -1;
    }

    for (unsigned int i = count; i > 0; i--) {
        unsigned int bit = (value >> (i - 1)) & 1U;

        if (bit_write_bit(writer, bit) != 0) {
            return -1;
        }
    }

    return 0;
}

/* Write any remaining bits, padding the final byte with zeros */
int bit_writer_flush(BitWriter *writer){
    if (writer == NULL || writer->file == NULL) {
        return -1;
    }

    if (writer->bit_count > 0) {
        writer->buffer = (uint8_t)(
            writer->buffer << (8U - writer->bit_count)
        );

        if (fputc(writer->buffer, writer->file) == EOF) {
            return -1;
        }

        writer->buffer = 0;
        writer->bit_count = 0;
    }

    return fflush(writer->file) == 0 ? 0 : -1;
}

/* Initialize the bit reader */
int bit_reader_init(BitReader *reader, FILE *file)
{
    if (reader == NULL || file == NULL) {
        return -1;
    }

    reader->file = file;
    reader->buffer = 0;
    reader->bit_count = 0;

    return 0;
}

/* Read a single bit */
int bit_read_bit(BitReader *reader, unsigned int *bit)
{
    if (reader == NULL || reader->file == NULL || bit == NULL) {
        return -1;
    }

    if (reader->bit_count == 0) {
        int byte = fgetc(reader->file);

        if (byte == EOF) {
            return -1;
        }

        reader->buffer = (uint8_t)byte;
        reader->bit_count = 8;
    }

    *bit = (reader->buffer >> (reader->bit_count - 1)) & 1U;
    reader->bit_count--;

    return 0;
}

/* Read multiple bits into a 32-bit value */
int bit_read_bits(BitReader *reader, unsigned int count, uint32_t *value)
{
    if (reader == NULL || reader->file == NULL ||
        value == NULL || count > 32) {
        return -1;
    }

    uint32_t result = 0;

    for (unsigned int i = 0; i < count; i++) {
        unsigned int bit;

        if (bit_read_bit(reader, &bit) != 0) {
            return -1;
        }

        result = (result << 1) | bit;
    }

    *value = result;

    return 0;
}