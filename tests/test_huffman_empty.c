#include <stdio.h>
#include <stdint.h>

#include "huffman.h"
#include "bitstream.h"

int main(void) {

    uint8_t *input = NULL;
    size_t input_size = 0;

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};
    uint16_t lengths[HUFFMAN_SYMBOL_COUNT] = {0};

    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT];

    HuffmanNode *root = NULL;

    FILE *file = tmpfile();

    BitWriter writer;
    BitReader reader;

    printf("Testing Empty Huffman Input...\n\n");

    if (file == NULL)
        return 1;

    /* Step 1: Frequency counting */

    if (huffman_count_frequencies(input, input_size, frequencies) != 0)
        goto error;

    printf("PASS: Empty frequency counting.\n");

    /* Step 2: Build Huffman tree */

    root = huffman_build_tree(frequencies);

    if (root != NULL)
        goto error;

    printf("PASS: Empty Huffman tree handling.\n");

    /* Step 3: Calculate code lengths */

    if (huffman_calculate_code_lengths(root, lengths) != 0)
        goto error;

    printf("PASS: Empty code-length calculation.\n");

    /* Step 4: Generate canonical codes */

    if (huffman_generate_canonical_codes(lengths, codes) != 0)
        goto error;

    printf("PASS: Empty canonical code generation.\n");

    /* Step 5: Initialize bit writer */

    if (bit_writer_init(&writer, file) != 0)
        goto error;

    if (bit_writer_flush(&writer) != 0)
        goto error;

    printf("PASS: Empty bitstream encoding.\n");

    /* Step 6: Initialize bit reader */

    rewind(file);

    if (bit_reader_init(&reader, file) != 0)
        goto error;

    printf("PASS: Empty bitstream reader initialization.\n");

    /* No symbols exist, so no decoding is necessary. */

    printf("PASS: No unnecessary decoding attempted.\n");

    printf("\nALL EMPTY INPUT TESTS PASSED!\n");

    fclose(file);

    return 0;

error:

    printf("FAIL: Empty Huffman test.\n");

    fclose(file);
    huffman_destroy_tree(root);

    return 1;
}