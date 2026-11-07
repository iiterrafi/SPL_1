#include <stdio.h>
#include <stdint.h>
#include "huffman.h"

int main() {

    uint8_t input[] = {'A', 'A', 'A', 'A', 'B', 'B', 'B', 'C', 'C', 'D'};

    size_t input_size = sizeof(input);

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};
    HuffmanNode *root = NULL;
    uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT] = {0};
    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT];

    printf("Testing Complete Huffman Pipeline...\n\n");

    /* Step 1: Count symbol frequencies */

    if(huffman_count_frequencies(input, input_size, frequencies) != 0) {
        printf("FAIL: Frequency counting.\n");
        return 1;
    }

    if(frequencies['A'] != 4 || frequencies['B'] != 3 || frequencies['C'] != 2 || frequencies['D'] != 1){
        printf("FAIL: Incorrect frequencies.\n");
        return 1;
    }

    printf("PASS: Frequency counting.\n");

    /* Step 2: Build Huffman Tree */

    root = huffman_build_tree(frequencies);

    if(root == NULL){
        printf("FAIL: Huffman tree construction.\n");
        return 1;
    }

    printf("PASS: Huffman tree construction.\n");

    /* Step 3: Calculate Huffman code lengths */

    if(huffman_calculate_code_lengths(root, code_lengths) != 0){
        printf("FAIL: Code-length calculation.\n");
        huffman_destroy_tree(root);
        return 1;
    }

    printf("PASS: Huffman code-length calculation.\n");

    /* Step 4: Generate Canonical Huffman codes */

    if(huffman_generate_canonical_codes(code_lengths, codes) != 0){
        printf("FAIL: Canonical code generation.\n");
        huffman_destroy_tree(root);
        return 1;
    }

    printf("PASS: Canonical code generation.\n");

    /* Step 5: Display generated Huffman codes */

    printf("\nGenerated Canonical Huffman Codes:\n");

    for(int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++){
        if(frequencies[i] > 0){
            printf("Symbol: %c | Frequency: %llu | Code: ", i, (unsigned long long)frequencies[i]);
            for(int bit = 0; bit < codes[i].length; bit++){
                int bit_position = codes[i].length - 1 - bit;
                int byte_index = 31 - (bit_position / 8);
                int offset = bit_position % 8;

                printf("%d", (codes[i].bits[byte_index] >> offset) & 1);
            }
            printf("\n");
        }
    }

    huffman_destroy_tree(root);

    return 0;
}