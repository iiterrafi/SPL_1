#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "huffman.h"
#include "bitstream.h"

int main(void) {

    uint8_t input[] = "AAAABBBCCD";
    size_t input_size = sizeof(input) - 1;

    uint8_t decoded[sizeof(input) - 1];

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};
    uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT] = {0};
    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT];

    HuffmanNode *root = NULL;

    FILE *file = NULL;

    BitWriter writer;
    BitReader reader;

    uint64_t total_bits = 0;

    printf("Testing Complete Huffman + Bitstream Integration...\n\n");

    /* Step 1: Count frequencies */

    if (huffman_count_frequencies(input, input_size, frequencies) != 0)
        return 1;

    /* Step 2: Build Huffman tree */

    root = huffman_build_tree(frequencies);

    if (root == NULL)
        return 1;

    /* Step 3: Calculate code lengths */

    if (huffman_calculate_code_lengths(root, code_lengths) != 0)
        goto error;

    /* Step 4: Generate canonical codes */

    if (huffman_generate_canonical_codes(code_lengths, codes) != 0)
        goto error;

    printf("PASS: Huffman code generation.\n");

    /* Step 5: Create temporary binary file */

    file = tmpfile();

    if (file == NULL)
        goto error;

    if (bit_writer_init(&writer, file) != 0)
        goto error;

    /* Step 6: Encode and write Huffman codes */

    for (size_t i = 0; i < input_size; i++) {

        HuffmanCode code = codes[input[i]];

        for (unsigned int bit = 0; bit < code.length; bit++) {

            int position = code.length - 1 - bit;
            int byte_index = 31 - position / 8;
            int offset = position % 8;

            unsigned int value =
                (code.bits[byte_index] >> offset) & 1U;

            if (bit_write_bit(&writer, value) != 0)
                goto error;

            total_bits++;
        }
    }

    if (bit_writer_flush(&writer) != 0)
        goto error;

    printf("PASS: Binary bitstream encoding and writing.\n");
    printf("Total meaningful bits: %llu\n",
           (unsigned long long)total_bits);

    /* Step 7: Return to beginning of binary file */

    rewind(file);

    if (bit_reader_init(&reader, file) != 0)
        goto error;

    printf("PASS: Binary bitstream reading initialized.\n");

    /* Step 8: Decode original symbols */

    for (size_t i = 0; i < input_size; i++) {

        if (huffman_decode_symbol(&reader, codes, &decoded[i]) != 0)
            goto error;
    }

    printf("PASS: Huffman decoding.\n");

    /* Step 9: Compare original and decoded data */

    if (memcmp(input, decoded, input_size) != 0) {
        printf("FAIL: Original and decoded data differ.\n");
        goto error;
    }

    printf("PASS: Original data matches reconstructed data.\n");

    printf("\nALL HUFFMAN + BITSTREAM INTEGRATION TESTS PASSED!\n");

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