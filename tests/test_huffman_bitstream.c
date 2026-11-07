#include <stdio.h>
#include <stdint.h>

#include "huffman.h"
#include "bitstream.h"

int main(){

    uint8_t input[] = "AAAABBBCCD";
    size_t input_size = sizeof(input) - 1;

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};
    uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT] = {0};
    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT];

    HuffmanNode *root = NULL;

    FILE *file = NULL;
    BitWriter writer;
    BitReader reader;

    uint64_t total_bits = 0;

    printf("Testing Huffman + Bitstream Integration...\n\n");

    /* Step 1: Generate Huffman codes */

    if (huffman_count_frequencies(input, input_size, frequencies) != 0)
        return 1;

    root = huffman_build_tree(frequencies);

    if (root == NULL)
        return 1;

    if (huffman_calculate_code_lengths(root, code_lengths) != 0)
        goto error;

    if (huffman_generate_canonical_codes(code_lengths, codes) != 0)
        goto error;

    printf("PASS: Huffman codes generated.\n");

    /* Step 2: Create temporary binary file */

    file = tmpfile();

    if (file == NULL)
        goto error;

    if (bit_writer_init(&writer, file) != 0)
        goto error;

    /* Step 3: Write Huffman codes as individual bits */

    for (size_t i = 0; i < input_size; i++) {

        HuffmanCode code = codes[input[i]];

        for (unsigned int bit = 0; bit < code.length; bit++) {

            int position = code.length - 1 - bit;
            int byte_index = 31 - position / 8;
            int offset = position % 8;

            unsigned int value =
                (code.bits[byte_index] >> offset) & 1;

            if (bit_write_bit(&writer, value) != 0)
                goto error;

            total_bits++;
        }
    }

    if (bit_writer_flush(&writer) != 0)
        goto error;

    printf("PASS: Huffman codes written to binary file.\n");
    printf("Total meaningful bits: %llu\n",
           (unsigned long long)total_bits);

    /* Step 4: Read the bits back */

    rewind(file);

    if (bit_reader_init(&reader, file) != 0)
        goto error;

    for (size_t i = 0; i < input_size; i++) {

        HuffmanCode code = codes[input[i]];

        for (unsigned int bit = 0; bit < code.length; bit++) {

            int position = code.length - 1 - bit;
            int byte_index = 31 - position / 8;
            int offset = position % 8;

            unsigned int expected =
                (code.bits[byte_index] >> offset) & 1;

            unsigned int actual;

            if (bit_read_bit(&reader, &actual) != 0)
                goto error;

            if (actual != expected) {
                printf("FAIL: Bit mismatch.\n");
                goto error;
            }
        }
    }

    printf("PASS: All bits read back correctly.\n");
    printf("\nALL HUFFMAN + BITSTREAM TESTS PASSED!\n");

    fclose(file);
    huffman_destroy_tree(root);

    return 0;

error:

    printf("FAIL: Huffman + Bitstream integration.\n");

    if (file != NULL)
        fclose(file);

    huffman_destroy_tree(root);

    return 1;
}