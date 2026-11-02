#include <stdio.h>
#include <stdint.h>
#include "huffman.h"

int main() {

    uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT] = {0};
    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT];

    printf("Testing Canonical Huffman Coding...\n\n");

    /* Test 1: Basic canonical code generation */

    code_lengths['A'] = 2;
    code_lengths['B'] = 2;
    code_lengths['C'] = 3;
    code_lengths['D'] = 3;

    if(huffman_generate_canonical_codes(code_lengths, codes) != 0) {
        printf("FAIL: Basic canonical code generation.\n");
        return 1;
    }

    if(codes['A'].length != 2 || codes['A'].bits[31] != 0 || codes['B'].length != 2 || codes['B'].bits[31] != 1 || codes['C'].length != 3 || codes['C'].bits[31] != 4 || codes['D'].length != 3 || codes['D'].bits[31] != 5){

        printf("FAIL: Incorrect canonical codes.\n");
        return 1;
    }

    printf("PASS: Basic canonical code generation.\n");
    
    /* Test 2: Symbols with equal lengths */

    for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        code_lengths[i] = 0;
    }

    code_lengths['C'] = 2;
    code_lengths['A'] = 2;
    code_lengths['B'] = 2;

    if(huffman_generate_canonical_codes(code_lengths, codes) != 0) {
        printf("FAIL: Equal-length symbol ordering.\n");
        return 1;
    }

    if (codes['A'].bits[31] != 0 || codes['B'].bits[31] != 1 || codes['C'].bits[31] != 2) {
        printf("FAIL: Incorrect symbol ordering.\n");
        return 1;
    }

    printf("PASS: Equal-length symbols ordered correctly.\n");

    /* Test 3: Multi-byte canonical code */

    for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        code_lengths[i] = 0;
    }

    code_lengths['A'] = 1;
    code_lengths['B'] = 9;

    if(huffman_generate_canonical_codes(code_lengths, codes) != 0) {
        printf("FAIL: Multi-byte canonical code generation.\n");
        return 1;
    }

    if(codes['A'].length != 1 ||
        codes['A'].bits[31] != 0 ||
        codes['B'].length != 9 ||
        codes['B'].bits[30] != 1 ||
        codes['B'].bits[31] != 0) {

        printf("FAIL: Incorrect multi-byte canonical codes.\n");
        return 1;
    }

    printf("PASS: Multi-byte canonical code generation.\n");

    /* Test 4: Empty code lengths */

    for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        code_lengths[i] = 0;
    }

    if (huffman_generate_canonical_codes(code_lengths, codes) != 0) {
        printf("FAIL: Empty code lengths.\n");
        return 1;
    }

    printf("PASS: Empty code lengths.\n");


    /* Test 5: Invalid arguments */

    if (huffman_generate_canonical_codes(NULL, codes) != -1 ||
        huffman_generate_canonical_codes(code_lengths, NULL) != -1) {

        printf("FAIL: Invalid argument handling.\n");
        return 1;
    }

    printf("PASS: Invalid argument handling.\n");


    /* Test 6: Oversubscribed code lengths */

    code_lengths['A'] = 1;
    code_lengths['B'] = 2;
    code_lengths['C'] = 2;
    code_lengths['D'] = 3;
    code_lengths['E'] = 3;

    if (huffman_generate_canonical_codes(code_lengths, codes) != -1) {
        printf("FAIL: Oversubscribed code lengths were accepted.\n");
        return 1;
    }

    printf("PASS: Oversubscribed code lengths rejected.\n");


    /* Test 7: Code length greater than 255 */

    for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        code_lengths[i] = 0;
    }

    code_lengths['A'] = 256;

    if (huffman_generate_canonical_codes(code_lengths, codes) != -1) {
        printf("FAIL: Invalid code length accepted.\n");
        return 1;
    }

    printf("PASS: Invalid code length rejected.\n");


    printf("\nAll Canonical Huffman tests passed!\n");

    return 0;
}