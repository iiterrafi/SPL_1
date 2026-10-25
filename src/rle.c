#include "rle.h"

#include <stdlib.h>
#include <string.h>

/*

* RLE packet format:
*
* Control byte: 0-127   -> literal packet containing control + 1 bytes
* Control byte: 128-255 -> run packet containing (control & 127) + 3
* ```
                      copies of the following byte
  ```
*
* Literal packet:
* control byte, followed by 1-128 literal bytes
*
* Run packet:
* control byte, followed by one repeated byte
  */

#define RLE_MAX_LITERAL 128
#define RLE_MIN_RUN 3
#define RLE_MAX_RUN 130

int rle_compress(const uint8_t *input, size_t input_size, uint8_t **output, size_t *output_size){
if (output == NULL || output_size == NULL || (input == NULL && input_size != 0)) {
return -1;
}

*output = NULL;
*output_size = 0;

if (input_size == 0) {
    return 0;
}

if (input_size > SIZE_MAX / 2) {
    return -1;
}

size_t capacity = input_size * 2;
uint8_t *compressed = malloc(capacity);

if (compressed == NULL) {
    return -1;
}

size_t in_pos = 0;
size_t out_pos = 0;

while (in_pos < input_size) {
    size_t run_length = 1;

    while (in_pos + run_length < input_size && input[in_pos + run_length] == input[in_pos] && run_length < RLE_MAX_RUN){
        run_length++;
    }

    if (run_length >= RLE_MIN_RUN) {
        compressed[out_pos++] = (uint8_t)(128 + run_length - RLE_MIN_RUN);
        compressed[out_pos++] = input[in_pos];

        in_pos += run_length;
    } else {
        size_t literal_start = in_pos;
        size_t literal_length = 0;

        while (in_pos < input_size && literal_length < RLE_MAX_LITERAL) {
            run_length = 1;
            while (in_pos + run_length < input_size && input[in_pos + run_length] == input[in_pos] && run_length < RLE_MAX_RUN) {
                run_length++;
            }

            if (run_length >= RLE_MIN_RUN) {
                break;
            }

            in_pos++;
            literal_length++;
        }

        compressed[out_pos++] = (uint8_t)(literal_length - 1);

        memcpy(&compressed[out_pos], &input[literal_start], literal_length);

        out_pos += literal_length;
    }
}

*output = compressed;
*output_size = out_pos;

return 0;

}

int rle_decompress(
const uint8_t *input,
size_t input_size,
uint8_t **output,
size_t *output_size
)
{
if (output == NULL || output_size == NULL ||
(input == NULL && input_size != 0)) {
return -1;
}

*output = NULL;
*output_size = 0;

if (input_size == 0) {
    return 0;
}

/*
 * First pass: validate packets and calculate the
 * exact decompressed size.
 */
size_t in_pos = 0;
size_t required_size = 0;

while (in_pos < input_size) {
    uint8_t control = input[in_pos++];

    if (control < 128) {
        size_t literal_length = (size_t)control + 1;

        if (literal_length > input_size - in_pos) {
            return -1;
        }

        if (literal_length > SIZE_MAX - required_size) {
            return -1;
        }

        required_size += literal_length;
        in_pos += literal_length;
    } else {
        size_t run_length =
            (size_t)(control & 127) + RLE_MIN_RUN;

        if (in_pos >= input_size) {
            return -1;
        }

        if (run_length > SIZE_MAX - required_size) {
            return -1;
        }

        required_size += run_length;
        in_pos++;
    }
}

/*
 * Allocate only the memory required for the output.
 */
if (required_size == 0) {
    return 0;
}

uint8_t *decompressed = malloc(required_size);

if (decompressed == NULL) {
    return -1;
}

/*
 * Second pass: decompress the validated packets.
 */
in_pos = 0;
size_t out_pos = 0;

while (in_pos < input_size) {
    uint8_t control = input[in_pos++];

    if (control < 128) {
        size_t literal_length = (size_t)control + 1;

        memcpy(
            &decompressed[out_pos],
            &input[in_pos],
            literal_length
        );

        in_pos += literal_length;
        out_pos += literal_length;
    } else {
        size_t run_length =
            (size_t)(control & 127) + RLE_MIN_RUN;

        uint8_t value = input[in_pos++];

        memset(
            &decompressed[out_pos],
            value,
            run_length
        );

        out_pos += run_length;
    }
}

*output = decompressed;
*output_size = out_pos;

return 0;

}

