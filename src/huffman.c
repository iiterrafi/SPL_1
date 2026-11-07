#include "huffman.h"
#include <stdlib.h>

/*
 * Count how many times each byte appears in the input.
 *
 * Returns:
 *   0  on success
 *  -1  on invalid arguments
 */

/*
 * Create a new Huffman tree node.
 */
static HuffmanNode *huffman_create_node(uint64_t frequency, int symbol, HuffmanNode *left, HuffmanNode *right) {
    HuffmanNode *node = malloc(sizeof(HuffmanNode));

    if (node == NULL) {
        return NULL;
    }

    node->frequency = frequency;
    node->symbol = symbol;
    node->left = left;
    node->right = right;

    return node;
}

/*
 * Recursively free a Huffman tree.
 */
static void huffman_free_tree(HuffmanNode *node) {
    if (node == NULL) {
        return;
    }

    huffman_free_tree(node->left);
    huffman_free_tree(node->right);

    free(node);
}

/*
 * Public function to destroy a Huffman tree.
 */
void huffman_destroy_tree(HuffmanNode *root) {
    huffman_free_tree(root);
}

static void huffman_calculate_lengths_recursive(const HuffmanNode *node, uint16_t depth, uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT]){
    if (node == NULL) {
        return;
    }

    /* Check whether this node is a leaf. */
    if (node->symbol != -1) {

        code_lengths[node->symbol] = depth;
        return;
    }

    /* Visit the left subtree. */
    huffman_calculate_lengths_recursive(
        node->left,
        depth + 1,
        code_lengths
    );

    /* Visit the right subtree. */
    huffman_calculate_lengths_recursive(
        node->right,
        depth + 1,
        code_lengths
    );
}

int huffman_calculate_code_lengths(const HuffmanNode *root, uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT]){
    if (code_lengths == NULL) {
        return -1;
    }

    /* Initialize all code lengths to zero. */
    for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        code_lengths[i] = 0;
    }

    /* Empty tree: no symbols to process. */
    if (root == NULL) {
        return 0;
    }

    /* Special case: only one symbol. */
    if (root->symbol != -1) {

        if (root->symbol < 0 ||
            root->symbol >= HUFFMAN_SYMBOL_COUNT) {
            return -1;
        }

        code_lengths[root->symbol] = 1;
        return 0;
    }

    /* Calculate lengths using recursive traversal. */
    huffman_calculate_lengths_recursive(
        root,
        0,
        code_lengths
    );

    return 0;
}

/*
 * Increment a 256-bit binary code by one.
 * Returns 0 on success and -1 on overflow.
 */
static int huffman_increment_code(uint8_t bits[32]) {

    for (int i = 31; i >= 0; i--) {

        if (bits[i] < 255) {
            bits[i]++;
            return 0;
        }

        bits[i] = 0;
    }

    return -1;
}

/*
 * Shift a 256-bit code left by one bit.
 * Returns 0 on success and -1 if the shift overflows.
 */
static int huffman_shift_code_left(uint8_t bits[32]) {

    uint8_t carry = 0;

    for (int i = 31; i >= 0; i--) {

        uint8_t new_carry = (uint8_t)(bits[i] >> 7);

        bits[i] = (uint8_t)((bits[i] << 1) | carry);

        carry = new_carry;
    }

    if (carry != 0) {
        return -1;
    }

    return 0;
}

int huffman_generate_canonical_codes(const uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT], HuffmanCode codes[HUFFMAN_SYMBOL_COUNT]){
    if (code_lengths == NULL || codes == NULL) {
        return -1;
    }

    int symbols[HUFFMAN_SYMBOL_COUNT];
    size_t symbol_count = 0;

    /* Initialize all output codes. */
    for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        codes[i].length = 0;

        for (int j = 0; j < 32; j++) {
            codes[i].bits[j] = 0;
        }

        if (code_lengths[i] > 255) {
            return -1;
        }
    }

    /* Arrange symbols by length, then by symbol value. */
    for (uint16_t length = 1; length <= 255; length++) {
        for (int symbol = 0; symbol < HUFFMAN_SYMBOL_COUNT; symbol++) {

            if (code_lengths[symbol] == length) {
                symbols[symbol_count++] = symbol;
            }
        }
    }

    /* Empty input: no codes to generate. */
    if (symbol_count == 0) {
        return 0;
    }

    uint8_t current_code[32] = {0};
    uint16_t previous_length = code_lengths[symbols[0]];

    for (size_t i = 0; i < symbol_count; i++) {

        int symbol = symbols[i];
        uint16_t current_length = code_lengths[symbol];

        if (i > 0) {

            /* Increment the previous code. */
            if (huffman_increment_code(current_code) != 0) {
                return -1;
            }

            /* Extend the code when its length increases. */
            for (uint16_t j = previous_length; j < current_length; j++) {
                if (huffman_shift_code_left(current_code) != 0) {
                    return -1;
                }
            }
        }

        /* Reject codes that exceed their specified length. */
        for (int bit = 0; bit < 256 - current_length; bit++) {

        int byte_index = bit / 8;
        int bit_position = 7 - (bit % 8);

        if (current_code[byte_index] & (1u << bit_position)) {
            return -1;
        }
    }

        codes[symbol].length = current_length;

        for (int j = 0; j < 32; j++) {
            codes[symbol].bits[j] = current_code[j];
        }

        previous_length = current_length;
    }

    return 0;
}

/*
 * Build a Huffman tree from symbol frequencies.
 */
HuffmanNode *huffman_build_tree(const uint64_t frequencies[HUFFMAN_SYMBOL_COUNT]) {
    /* Validate the frequency array. */
    if (frequencies == NULL) {
        return NULL;
    }

    /* Create a min-heap for at most 256 leaf nodes. */
    HuffmanMinHeap *heap =
        huffman_heap_create(HUFFMAN_SYMBOL_COUNT);

    if (heap == NULL) {
        return NULL;
    }

    /* Create leaf nodes for symbols with nonzero frequencies. */
    for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {

        if (frequencies[i] == 0) {
            continue;
        }

        HuffmanNode *node = huffman_create_node(
            frequencies[i],
            i,
            NULL,
            NULL
        );

        if (node == NULL) {
            for (size_t j = 0; j < heap->size; j++) {
                huffman_free_tree(heap->nodes[j]);
            }

            huffman_heap_destroy(heap);
            return NULL;
        }

        if (huffman_heap_insert(heap, node) != 0) {
            huffman_free_tree(node);

            for (size_t j = 0; j < heap->size; j++) {
                huffman_free_tree(heap->nodes[j]);
            }

            huffman_heap_destroy(heap);
            return NULL;
        }
    }

        /*
     * Continue combining nodes until only
     * one node remains in the heap.
     */
    while (heap->size > 1) {

        /* Extract the two lowest-frequency nodes. */
        HuffmanNode *left = huffman_heap_extract_min(heap);
        HuffmanNode *right = huffman_heap_extract_min(heap);

        /* Check for extraction failure. */
        if (left == NULL || right == NULL) {
            huffman_free_tree(left);
            huffman_free_tree(right);

            for (size_t i = 0; i < heap->size; i++) {
                huffman_free_tree(heap->nodes[i]);
            }

            huffman_heap_destroy(heap);
            return NULL;
        }

        /* Check for frequency addition overflow. */
        if (UINT64_MAX - left->frequency < right->frequency) {
            huffman_free_tree(left);
            huffman_free_tree(right);

            for (size_t i = 0; i < heap->size; i++) {
                huffman_free_tree(heap->nodes[i]);
            }

            huffman_heap_destroy(heap);
            return NULL;
        }

        /* Create a parent node from the two children. */
        HuffmanNode *parent = huffman_create_node(
            left->frequency + right->frequency,
            -1,
            left,
            right
        );

        if (parent == NULL) {
            huffman_free_tree(left);
            huffman_free_tree(right);

            for (size_t i = 0; i < heap->size; i++) {
                huffman_free_tree(heap->nodes[i]);
            }

            huffman_heap_destroy(heap);
            return NULL;
        }

        /* Insert the parent back into the min-heap. */
        if (huffman_heap_insert(heap, parent) != 0) {
            huffman_free_tree(parent);

            for (size_t i = 0; i < heap->size; i++) {
                huffman_free_tree(heap->nodes[i]);
            }

            huffman_heap_destroy(heap);
            return NULL;
        }
    }

        /* The remaining node is the root of the Huffman tree. */
    HuffmanNode *root = NULL;

    if (heap->size == 1) {
        root = huffman_heap_extract_min(heap);
    }

    /* Release the heap structure. */
    huffman_heap_destroy(heap);

    return root;
    
}

int huffman_count_frequencies(const uint8_t *input, size_t input_size, uint64_t frequencies[HUFFMAN_SYMBOL_COUNT]){
    /* The frequency array must exist. */
    if (frequencies == NULL) {
        return -1;
    }

    /* Non-empty input requires a valid input pointer. */
    if (input == NULL && input_size > 0) {
        return -1;
    }

    /* Initialize all 256 frequency counters to zero. */
    for (size_t i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {
        frequencies[i] = 0;
    }

    /* Count each byte. */
    for (size_t i = 0; i < input_size; i++) {
        frequencies[input[i]]++;
    }

    return 0;
}

/*
 * Create a min-heap with the specified capacity.
 *
 * Returns:
 *   A pointer to the heap on success.
 *   NULL on failure or if capacity is zero.
 */
HuffmanMinHeap *huffman_heap_create(size_t capacity) {
    /* A heap with zero capacity is not useful. */
    if (capacity == 0) {
        return NULL;
    }

    /* Allocate memory for the heap structure. */
    HuffmanMinHeap *heap = malloc(sizeof(HuffmanMinHeap));

    if (heap == NULL) {
        return NULL;
    }

    if (capacity > SIZE_MAX / sizeof(HuffmanNode *)) {
        free(heap);
        return NULL;
    }

    /* Allocate an array to store node pointers. */
    heap->nodes = malloc(capacity * sizeof(HuffmanNode *));

    if (heap->nodes == NULL) {
        free(heap);
        return NULL;
    }

    /* Initialize the heap's fields. */
    heap->size = 0;
    heap->capacity = capacity;

    return heap;
}

/*
 * Insert a node into the min-heap.
 *
 * Returns:
 *   0  on success
 *  -1  on invalid arguments or a full heap
 */

int huffman_heap_insert(HuffmanMinHeap *heap, HuffmanNode *node){
    /* Validate the arguments. */
    if (heap == NULL || node == NULL) {
        return -1;
    }

    /* Do not insert if the heap is full. */
    if (heap->size >= heap->capacity) {
        return -1;
    }

    /* Place the new node at the next available position. */
    size_t index = heap->size;
    heap->nodes[index] = node;
    heap->size++;

    /*
     * Sift the node upward until the min-heap
     * property is restored.
     */
    while (index > 0) {
        size_t parent = (index - 1) / 2;

        /* The parent is already smaller or equal. */
        if (heap->nodes[parent]->frequency <=
            heap->nodes[index]->frequency) {
            break;
        }

        /* Swap the parent and child pointers. */
        HuffmanNode *temp = heap->nodes[parent];
        heap->nodes[parent] = heap->nodes[index];
        heap->nodes[index] = temp;

        /* Continue from the parent's former position. */
        index = parent;
    }

    return 0;
}

/*
 * Remove and return the node with the smallest frequency.
 *
 * Returns:
 *   Pointer to the minimum node on success.
 *   NULL if the heap is empty or invalid.
 */
HuffmanNode *huffman_heap_extract_min(HuffmanMinHeap *heap) {
    if (heap == NULL || heap->size == 0) {
        return NULL;
    }

    /* The root always contains the minimum-frequency node. */
    HuffmanNode *min_node = heap->nodes[0];

    /* Move the last node to the root. */
    heap->size--;

    if (heap->size == 0) {
        return min_node;
    }

    heap->nodes[0] = heap->nodes[heap->size];

    /* Restore the min-heap property by sifting downward. */
    size_t index = 0;

    while (1) {
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t smallest = index;

        /* Check whether the left child is smaller. */
        if (left < heap->size &&
            heap->nodes[left]->frequency <
            heap->nodes[smallest]->frequency) {
            smallest = left;
        }

        /* Check whether the right child is smaller. */
        if (right < heap->size &&
            heap->nodes[right]->frequency <
            heap->nodes[smallest]->frequency) {
            smallest = right;
        }

        /* The current node is already the smallest. */
        if (smallest == index) {
            break;
        }

        /* Swap the current node with the smallest child. */
        HuffmanNode *temp = heap->nodes[index];
        heap->nodes[index] = heap->nodes[smallest];
        heap->nodes[smallest] = temp;

        index = smallest;
    }

    return min_node;
}

/*
 * Destroy the min-heap.
 *
 * Frees the heap array and heap structure.
 * Does not free individual Huffman nodes.
 */
void huffman_heap_destroy(HuffmanMinHeap *heap) {
    if (heap == NULL) {
        return;
    }

    free(heap->nodes);
    free(heap);
}

int huffman_decode_symbol(
    BitReader *reader,
    const HuffmanCode codes[HUFFMAN_SYMBOL_COUNT],
    uint8_t *symbol
) {
    if (reader == NULL || codes == NULL || symbol == NULL) {
        return -1;
    }

    uint8_t current_bits[32] = {0};

    for (unsigned int length = 1; length <= 255; length++) {

        unsigned int bit;

        if (bit_read_bit(reader, &bit) != 0) {
            return -1;
        }

        /* Append the new bit to the current code. */
        for (int i = 0; i < 31; i++) {
            current_bits[i] = (uint8_t)((current_bits[i] << 1) | (current_bits[i + 1] >> 7));
        }

        current_bits[31] <<= 1;

        current_bits[31] |= (uint8_t)bit;

        /* Search for a matching canonical code. */
        for (int i = 0; i < HUFFMAN_SYMBOL_COUNT; i++) {

            if (codes[i].length != length) {
                continue;
            }

            int match = 1;

            for (int j = 0; j < 32; j++) {
                if (current_bits[j] != codes[i].bits[j]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                *symbol = (uint8_t)i;
                return 0;
            }
        }
    }

    return -1;
}
