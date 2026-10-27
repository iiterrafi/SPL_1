#include "lz77.h"

#include <stdlib.h>
#include <string.h>

/* Number of entries in the hash table */
#define LZ77_HASH_SIZE 65536

/* Hash-chain data structure */
typedef struct {
size_t *head;
size_t *previous;
size_t input_size;
} LZ77HashChain;

/* Calculate a hash from three consecutive bytes */
static size_t hash_three_bytes(const uint8_t *input){
    size_t hash = (size_t)input[0] * 251u + (size_t)input[1] * 31u + (size_t)input[2];

    return hash % LZ77_HASH_SIZE;
}

/* Initialize the hash-chain data structure */
static int hash_chain_init(LZ77HashChain *chain, size_t input_size){
    chain->head = NULL;
    chain->previous = NULL;
    chain->input_size = input_size;

    /* Allocate memory for the hash table */
    chain->head = malloc(LZ77_HASH_SIZE * sizeof(size_t));

    if (chain->head == NULL) {
        return -1;
    }

    /* Mark every hash-table entry as empty */
    for (size_t i = 0; i < LZ77_HASH_SIZE; i++) {
        chain->head[i] = SIZE_MAX;
    }

    /* Empty input requires no position-link array */
    if (input_size == 0) {
        return 0;
    }

    /* Check whether the allocation size would overflow */
    if (input_size > SIZE_MAX / sizeof(size_t)) {
        free(chain->head);
        chain->head = NULL;
        return -1;
    }

    /* Allocate links for input positions */
    chain->previous = malloc(input_size * sizeof(size_t));

    if (chain->previous == NULL) {
        free(chain->head);
        chain->head = NULL;
        return -1;
    }

    /* Initially, every position has no previous link */
    for (size_t i = 0; i < input_size; i++) {
        chain->previous[i] = SIZE_MAX;
    }

    return 0;
}

/* Release memory used by the hash chain */
static void hash_chain_free(LZ77HashChain *chain){
    free(chain->head);
    free(chain->previous);

    chain->head = NULL;
    chain->previous = NULL;
    chain->input_size = 0;
}

/* Insert an input position into the hash chain */
static void hash_chain_insert(LZ77HashChain *chain, const uint8_t *input, size_t position){
    /* Reject positions that cannot contain three bytes */
    if (position > chain->input_size || chain->input_size - position < 3) {
        return;
    }

    /* Calculate the hash for the three-byte sequence */
    size_t hash = hash_three_bytes(input + position);

    /* Link this position to the previous position with the same hash */
    chain->previous[position] = chain->head[hash];

    /* Make this position the newest entry */
    chain->head[hash] = position;
}

/* Find the longest previous match using the hash chain */
static void hash_chain_find_match(const LZ77HashChain *chain, const uint8_t *input,size_t position, size_t max_chain_length, size_t *best_distance, size_t *best_length){
    *best_distance = 0;
    *best_length = 0;

    /* At least three bytes must remain for a match */
    if (chain->input_size - position < LZ77_MIN_MATCH) {
        return;
    }

    size_t hash = hash_three_bytes(input + position);
    size_t candidate = chain->head[hash];
    size_t searched = 0;

    size_t remaining = chain->input_size - position;
    size_t max_length = remaining < LZ77_MAX_MATCH ? remaining : LZ77_MAX_MATCH;

    while (candidate != SIZE_MAX && searched < max_chain_length) {

        /* Candidates must be earlier than the current position */
        if (candidate >= position) {
            break;
        }

        size_t distance = position - candidate;

        /* Stop when candidates exceed the sliding window */
        if (distance > LZ77_WINDOW_SIZE) {
            break;
        }

        size_t length = 0;

        /* Compare bytes to find the matching sequence */
        while (length < max_length && input[candidate + length] == input[position + length]) {
            length++;
        }

        if (length > *best_length) {
            *best_length = length;
            *best_distance = distance;

            /* A maximum-length match cannot be improved */
            if (length == max_length) {
                break;
            }
        }

        candidate = chain->previous[candidate];
        searched++;
    }

    /* Discard matches shorter than the minimum */
    if (*best_length < LZ77_MIN_MATCH) {
        *best_length = 0;
        *best_distance = 0;
    }
}


/* Store a token representing a match and an optional next byte */
static void lz77_set_token(LZ77Token *token, size_t distance, size_t length, uint8_t next_byte, int has_match, int has_next_byte){
    token->distance = distance;
    token->length = length;
    token->next_byte = next_byte;
    token->has_match = has_match;
    token->has_next_byte = has_next_byte;
}

/* Compress input data using LZ77 */
int lz77_compress(const uint8_t *input, size_t input_size, LZ77Token **tokens, size_t *token_count, size_t max_chain_length){
    /* Validate output parameters */
    if(tokens == NULL || token_count == NULL) {
        return -1;
    }

    *tokens = NULL;
    *token_count = 0;

    /* Validate input parameters */
    if ((input == NULL && input_size > 0) ||
        max_chain_length == 0) {
        return -1;
    }

    /* Empty input produces no tokens */
    if (input_size == 0) {
        return 0;
    }

    /* Check allocation size for the token array */
    if (input_size > SIZE_MAX / sizeof(LZ77Token)) {
        return -1;
    }

    /* Allocate enough space for the worst case */
    LZ77Token *result = malloc(input_size * sizeof(LZ77Token));

    if (result == NULL) {
        return -1;
    }

    /* Initialize the hash chain */
    LZ77HashChain chain;

    if (hash_chain_init(&chain, input_size) != 0) {
        free(result);
        return -1;
    }

    size_t position = 0;
    size_t count = 0;

    while (position < input_size) {
        size_t best_distance = 0;
        size_t best_length = 0;

        /* Search for a previous match */
        hash_chain_find_match(&chain, input, position, max_chain_length, &best_distance, &best_length);

        size_t consumed;

        if (best_length >= LZ77_MIN_MATCH) {
            /* Determine whether a literal byte follows the match */
            int has_next_byte = best_length < input_size - position;
            uint8_t next_byte = 0;

            if(has_next_byte){
                next_byte = input[position + best_length];
            }

            /* Create a match token */
            lz77_set_token(&result[count], best_distance, best_length, next_byte, 1, has_next_byte);
            consumed = best_length + (size_t)has_next_byte;
        } else {
            /* No useful match: store one literal byte */
            lz77_set_token(&result[count], 0, 0, input[position], 0, 1);
            consumed = 1;
        }

        count++;

        /* Insert every consumed position into the hash chain */
        for(size_t i = 0; i < consumed; i++) {
            hash_chain_insert(&chain, input, position + i);
        }

        position += consumed;
    }

    /* Release temporary hash-chain memory */
    hash_chain_free(&chain);

    *tokens = result;
    *token_count = count;
    
    return 0;
}

/* Decompress LZ77 tokens into the original byte sequence */
int lz77_decompress(const LZ77Token *tokens, size_t token_count, uint8_t **output, size_t *output_size){
    /* Validate output parameters */ 
    if (output == NULL || output_size == NULL) {
        return -1;
    }

    *output = NULL;
    *output_size = 0;

    /* Validate token input */
    if (tokens == NULL && token_count > 0) {
        return -1;
    }

    /*
    * First pass:
    * Validate tokens and calculate the required output size.
    */
    size_t required_size = 0;

    for (size_t i = 0; i < token_count; i++) {
        const LZ77Token *token = &tokens[i];

        /* Boolean fields must contain only 0 or 1 */
        if ((token->has_match != 0 && token->has_match != 1) || (token->has_next_byte != 0 && token->has_next_byte != 1)) {
            return -1;
        }

        if (token->has_match) {
            /* Validate the match length */
            if (token->length < LZ77_MIN_MATCH || token->length > LZ77_MAX_MATCH || token->distance == 0 || token->distance > LZ77_WINDOW_SIZE || token->distance > required_size) {
                return -1;
            }

            /* Check for size overflow */
            if (token->length > SIZE_MAX - required_size) {
                return -1;
            }

            required_size += token->length;
        }else{
            /* A literal-only token must have zero match fields */
            if(token->length != 0 || token->distance != 0 || !token->has_next_byte) {
                return -1;
            }
        }

        /* Add the optional following literal byte */
        if (token->has_next_byte) {
            if (required_size == SIZE_MAX) {
                return -1;
            }

            required_size++;
        }
    }

    /* Empty output requires no allocation */
    if (required_size == 0) {
        return 0;
    }

    /* Allocate the exact output size */
    uint8_t *result = malloc(required_size);

    if (result == NULL) {
        return -1;
    }

    /*
    * Second pass:
    * Reconstruct the original data.
    */
    size_t position = 0;

    for (size_t i = 0; i < token_count; i++) {
        const LZ77Token *token = &tokens[i];

        if (token->has_match) {
            for (size_t j = 0; j < token->length; j++) {
                result[position] = result[position - token->distance];
                position++;
            }
        }

        if (token->has_next_byte) {
            result[position] = token->next_byte;
            position++;
        }
    }

    *output = result;
    *output_size = required_size;

    return 0;

}
