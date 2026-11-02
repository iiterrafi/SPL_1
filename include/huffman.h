#ifndef HUFFMAN_H
#define HUFFMAN_H


#include <stddef.h>
#include <stdint.h>

/* Number of possible byte values: 0 through 255 */
#define HUFFMAN_SYMBOL_COUNT 256

/*
 * A node in the Huffman tree.
 *
 * Leaf nodes represent individual byte values.
 * Internal nodes combine two child nodes.
 */
typedef struct HuffmanNode {
    uint64_t frequency;
    int symbol;

    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

typedef struct {
    uint8_t bits[32];
    uint16_t length;
} HuffmanCode;

/*
 * Min-heap for Huffman tree nodes.
 *
 * The node with the smallest frequency
 * is maintained at the top of the heap.
 */
typedef struct {
    HuffmanNode **nodes;
    size_t size;
    size_t capacity;
} HuffmanMinHeap;

/* Create a min-heap with the specified capacity. */
HuffmanMinHeap *huffman_heap_create(size_t capacity);

/* Insert a node while maintaining the min-heap property. */
int huffman_heap_insert(HuffmanMinHeap *heap, HuffmanNode *node);

/* Remove and return the node with the smallest frequency. */
HuffmanNode *huffman_heap_extract_min(HuffmanMinHeap *heap);

/* Release the heap's internal array and heap structure. */
void huffman_heap_destroy(HuffmanMinHeap *heap);

/*
 * Build a Huffman tree using the given symbol frequencies.
 *
 * Returns:
 *   Pointer to the root node on success.
 *   NULL if the input is empty or memory allocation fails.
 */
HuffmanNode *huffman_build_tree(const uint64_t frequencies[HUFFMAN_SYMBOL_COUNT]);

/* Free an entire Huffman tree. */
void huffman_destroy_tree(HuffmanNode *root);

int huffman_calculate_code_lengths(const HuffmanNode *root, uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT]);

int huffman_generate_canonical_codes(const uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT], HuffmanCode codes[HUFFMAN_SYMBOL_COUNT]);

/* Count the frequency of each byte */
int huffman_count_frequencies(const uint8_t *input, size_t input_size, uint64_t frequencies[HUFFMAN_SYMBOL_COUNT]);

#endif