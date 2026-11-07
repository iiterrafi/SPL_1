#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "huffman.h"
#include "bitstream.h"

int main() {

    uint8_t input[] = {'A', 'A', 'A', 'A', 'A', 'A'};
    size_t input_size = sizeof(input);

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};
    uint16_t lengths[HUFFMAN_SYMBOL_COUNT] = {0};

    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT];

    HuffmanNode *root = NULL;

    uint8_t decoded[sizeof(input)] = {0};

    FILE *file = tmpfile();

    BitWriter writer;
    BitReader reader;

    printf("Testing Single-Symbol Huffman...\n\n");

    if (file == NULL) {
        printf("FAIL: Temporary file creation.\n");
        return 1;
    }
    /* Step 1: Count frequencies */

    if (huffman_count_frequencies(input, input_size, frequencies) != 0)
        goto error;

    if (frequencies['A'] != 6) {
        printf("FAIL: Incorrect frequency.\n");
        goto error;
    }

    printf("PASS: Frequency counting.\n");

    /* Step 2: Build Huffman tree */

    root = huffman_build_tree(frequencies);

    if (root == NULL)
        goto error;

    printf("PASS: Huffman tree construction.\n");

    /* Step 3: Calculate code lengths */

    if (huffman_calculate_code_lengths(root, lengths) != 0)
        goto error;

    if (lengths['A'] != 1) {
        printf("FAIL: Incorrect single-symbol code length.\n");
        goto error;
    }

    printf("PASS: Code-length calculation.\n");

    /* Step 4: Generate canonical codes */

    if (huffman_generate_canonical_codes(lengths, codes) != 0)
        goto error;

    printf("PASS: Canonical code generation.\n");

    /* Step 5: Encode */

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

    printf("PASS: Encoding.\n");

    /* Step 6: Decode */

    rewind(file);

    if (bit_reader_init(&reader, file) != 0)
        goto error;

    for (size_t i = 0; i < input_size; i++) {
        if (huffman_decode_symbol(&reader, codes, &decoded[i]) != 0)
        goto error;
    }

    printf("PASS: Decoding.\n");

    /* Step 7: Compare */

    if (memcmp(input, decoded, input_size) != 0) {
        printf("FAIL: Original and decoded data differ.\n");
        goto error;
    }

    printf("PASS: Original and decoded data match.\n");

    printf("\nALL SINGLE-SYMBOL TESTS PASSED!\n");

    fclose(file);
    huffman_destroy_tree(root);

    return 0;

    error:

    printf("FAIL: Single-symbol Huffman test.\n");

    fclose(file);
    huffman_destroy_tree(root);

    return 1;

}