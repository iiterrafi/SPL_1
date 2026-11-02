#include "huffman.h"
#include <stdio.h>
#include <stdint.h>

/*
 * Test Huffman tree construction with three symbols.
 */
void test_basic_tree(){

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};

    frequencies['A'] = 5;
    frequencies['B'] = 2;
    frequencies['C'] = 1;

    HuffmanNode *root = huffman_build_tree(frequencies);

    if (root == NULL) {
        printf("FAIL: Huffman tree construction failed.\n");
        return;
    }

    /* The root frequency must equal the total frequency. */
    if (root->frequency != 8) {
        printf("FAIL: Incorrect root frequency.\n");
        huffman_destroy_tree(root);
        return;
    }

    /* An internal root must not represent an individual symbol. */
    if (root->symbol != -1) {
        printf("FAIL: Root should be an internal node.\n");
        huffman_destroy_tree(root);
        return;
    }

    printf("PASS: Basic Huffman tree construction.\n");

    huffman_destroy_tree(root);
}

/*
 * Test Huffman tree construction with empty input.
 */
void test_empty_tree(){

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};

    HuffmanNode *root = huffman_build_tree(frequencies);

    if (root != NULL) {
        printf("FAIL: Empty input should produce NULL.\n");
        huffman_destroy_tree(root);
        return;
    }

    printf("PASS: Empty input produces an empty tree.\n");
}

/*
 * Test Huffman tree construction with one unique symbol.
 */
void test_single_symbol_tree(){

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};

    frequencies['A'] = 7;

    HuffmanNode *root = huffman_build_tree(frequencies);

    if (root == NULL) {
        printf("FAIL: Single-symbol tree construction failed.\n");
        return;
    }

    if (root->frequency != 7 || root->symbol != 'A') {
        printf("FAIL: Incorrect single-symbol tree.\n");
        huffman_destroy_tree(root);
        return;
    }

    if (root->left != NULL || root->right != NULL) {
        printf("FAIL: Single-symbol tree should be a leaf.\n");
        huffman_destroy_tree(root);
        return;
    }

    printf("PASS: Single-symbol tree construction.\n");

    huffman_destroy_tree(root);
}

/*
 * Test Huffman tree construction with multiple symbols.
 */
void test_multiple_symbols_tree() {

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};

    frequencies['A'] = 10;
    frequencies['B'] = 5;
    frequencies['C'] = 3;
    frequencies['D'] = 2;

    HuffmanNode *root = huffman_build_tree(frequencies);

    if (root == NULL) {
        printf("FAIL: Multiple-symbol tree construction failed.\n");
        return;
    }

    /* The root frequency must equal the total frequency. */
    if (root->frequency != 20) {
        printf("FAIL: Incorrect total root frequency.\n");
        huffman_destroy_tree(root);
        return;
    }

    /* The root must be an internal node. */
    if (root->symbol != -1) {
        printf("FAIL: Root should be an internal node.\n");
        huffman_destroy_tree(root);
        return;
    }

    /* An internal node must have two children. */
    if (root->left == NULL || root->right == NULL) {
        printf("FAIL: Root must have two children.\n");
        huffman_destroy_tree(root);
        return;
    }

    printf("PASS: Multiple-symbol tree construction.\n");

    huffman_destroy_tree(root);
}

/*
 * Test Huffman tree construction with invalid arguments.
 */
void test_invalid_arguments(void) {

    HuffmanNode *root = huffman_build_tree(NULL);

    if (root != NULL) {
        printf("FAIL: NULL frequency array should return NULL.\n");
        huffman_destroy_tree(root);
        return;
    }

    printf("PASS: Invalid argument handling.\n");
}

/*
 * Test Huffman code-length calculation.
 */
void test_code_lengths() {

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};

    frequencies['A'] = 5;
    frequencies['B'] = 2;
    frequencies['C'] = 1;

    HuffmanNode *root = huffman_build_tree(frequencies);

    if (root == NULL) {
        printf("FAIL: Could not construct Huffman tree.\n");
        return;
    }

    uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT];

    int result = huffman_calculate_code_lengths(
        root,
        code_lengths
    );

    if (result != 0) {
        printf("FAIL: Code-length calculation failed.\n");
        huffman_destroy_tree(root);
        return;
    }

    if (code_lengths['A'] != 1 ||
        code_lengths['B'] != 2 ||
        code_lengths['C'] != 2) {

        printf("FAIL: Incorrect Huffman code lengths.\n");

        huffman_destroy_tree(root);
        return;
    }

    printf("PASS: Huffman code-length calculation.\n");

    huffman_destroy_tree(root);
}

int main() {

    printf("Testing Huffman tree construction...\n\n");

    test_basic_tree();
    test_empty_tree();
    test_single_symbol_tree();
    test_multiple_symbols_tree();
    test_invalid_arguments();
    test_code_lengths();
    
    return 0;
}