#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "huffman.h"
#include "bitstream.h"

int main() {

    uint8_t input[] = "AAAABBBCCD";
    size_t input_size = sizeof(input) - 1;

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};
    uint16_t lengths[HUFFMAN_SYMBOL_COUNT] = {0};
    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT];

    HuffmanNode *root = NULL;

    FILE *file = tmpfile();

    BitWriter writer;
    BitReader reader;

    uint8_t decoded[sizeof(input)] = {0};

    printf("Testing Huffman Encoding and Decoding...\n\n");

    if (file == NULL)
        return 1;

    /* Generate Huffman codes */

    huffman_count_frequencies(input, input_size, frequencies);

    root = huffman_build_tree(frequencies);

    if (root == NULL)
        goto error;

    if (huffman_calculate_code_lengths(root, lengths) != 0)
        goto error;

    if (huffman_generate_canonical_codes(lengths, codes) != 0)
        goto error;

    printf("PASS: Huffman codes generated.\n");

    /* Encode */

    if (bit_writer_init(&writer, file) != 0)
        goto error;

    for (size_t i = 0; i < input_size; i++) {

        HuffmanCode code = codes[input[i]];

        for (unsigned int j = 0; j < code.length; j++) {

            int position = code.length - 1 - j;
            int byte_index = 31 - position / 8;
            int offset = position % 8;

            unsigned int bit =
                (code.bits[byte_index] >> offset) & 1;

            if (bit_write_bit(&writer, bit) != 0)
                goto error;
        }
    }

    if (bit_writer_flush(&writer) != 0)
        goto error;

    printf("PASS: Encoding completed.\n");

    /* Decode */

    rewind(file);

    if (bit_reader_init(&reader, file) != 0)
        goto error;

    for (size_t i = 0; i < input_size; i++) {

        if (huffman_decode_symbol(&reader, codes, &decoded[i]) != 0)
            goto error;
    }

    printf("PASS: Decoding completed.\n");

    /* Compare original and decoded data */

    if (memcmp(input, decoded, input_size) != 0) {
        printf("FAIL: Original and decoded data differ.\n");
        goto error;
    }

    printf("PASS: Original and decoded data match.\n");

    printf("\nOriginal: ");
    fwrite(input, 1, input_size, stdout);

    printf("\nDecoded:  ");
    fwrite(decoded, 1, input_size, stdout);

    printf("\n\nALL HUFFMAN ROUND-TRIP TESTS PASSED!\n");

    fclose(file);
    huffman_destroy_tree(root);

    return 0;

error:

    printf("FAIL: Huffman round-trip test.\n");

    fclose(file);
    huffman_destroy_tree(root);

    return 1;
}