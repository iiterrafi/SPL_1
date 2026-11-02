#include <stdio.h>
#include <stdlib.h>
#include "huffman.h"

/* Test heap creation */
static int test_heap_creation(void) {
    printf("\nTesting heap creation...\n");

    HuffmanMinHeap *heap = huffman_heap_create(10);

    if (heap == NULL) {
        printf("FAIL: Heap creation failed.\n");
        return 0;
    }

    if (heap->size != 0 || heap->capacity != 10) {
        printf("FAIL: Incorrect heap initialization.\n");
        huffman_heap_destroy(heap);
        return 0;
    }

    huffman_heap_destroy(heap);

    printf("PASS: Heap created and destroyed.\n");
    return 1;
}

/* Test insertion and extraction */
static int test_heap_insert_extract(void) {
    printf("\nTesting heap insertion and extraction...\n");

    HuffmanMinHeap *heap = huffman_heap_create(5);

    if (heap == NULL) {
        return 0;
    }

    HuffmanNode nodes[4] = {
        {5, 'A', NULL, NULL},
        {2, 'B', NULL, NULL},
        {7, 'C', NULL, NULL},
        {1, 'D', NULL, NULL}
    };

    for (size_t i = 0; i < 4; i++) {
        if (huffman_heap_insert(heap, &nodes[i]) != 0) {
            printf("FAIL: Insertion failed.\n");
            huffman_heap_destroy(heap);
            return 0;
        }
    }

    uint64_t expected[] = {1, 2, 5, 7};

    for (size_t i = 0; i < 4; i++) {
        HuffmanNode *node = huffman_heap_extract_min(heap);

        if (node == NULL || node->frequency != expected[i]) {
            printf("FAIL: Incorrect extraction order.\n");
            huffman_heap_destroy(heap);
            return 0;
        }
    }

    huffman_heap_destroy(heap);

    printf("PASS: Nodes extracted in ascending frequency order.\n");
    return 1;
}

/* Test invalid operations */
static int test_invalid_operations(void) {
    printf("\nTesting invalid heap operations...\n");

    if (huffman_heap_create(0) != NULL) {
        printf("FAIL: Zero-capacity heap was accepted.\n");
        return 0;
    }

    HuffmanMinHeap *heap = huffman_heap_create(1);

    if (heap == NULL) {
        return 0;
    }

    HuffmanNode node = {1, 'A', NULL, NULL};

    if (huffman_heap_insert(NULL, &node) != -1 ||
        huffman_heap_insert(heap, NULL) != -1 ||
        huffman_heap_insert(heap, &node) != 0 ||
        huffman_heap_insert(heap, &node) != -1) {
        printf("FAIL: Invalid insertion handling.\n");
        huffman_heap_destroy(heap);
        return 0;
    }

    if (huffman_heap_extract_min(heap) != &node ||
        huffman_heap_extract_min(heap) != NULL) {
        printf("FAIL: Empty heap extraction handling.\n");
        huffman_heap_destroy(heap);
        return 0;
    }

    huffman_heap_destroy(heap);

    printf("PASS: Invalid heap operations handled correctly.\n");
    return 1;
}

int main(void) {
    printf("Testing Huffman min-heap operations...\n");

    if (!test_heap_creation() ||
        !test_heap_insert_extract() ||
        !test_invalid_operations()) {
        return 1;
    }

    printf("\nALL HUFFMAN HEAP TESTS PASSED!\n");
    return 0;
}