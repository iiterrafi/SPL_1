#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "compressor.h"
#include "rle.h"
#include "lz77.h"
#include "token_codec.h"
#include "huffman.h"
#include "bitstream.h"
#include "crc32.h"


/* Encode one byte using its canonical Huffman code. */
static int encode_symbol(BitWriter *writer, const HuffmanCode *code){
    if (writer == NULL || code == NULL || code->length == 0)
        return -1;

    for (unsigned int bit = 0; bit < code->length; bit++){
        unsigned int position = code->length - 1U - bit;
        unsigned int byte_index = 31U - position / 8U;
        unsigned int offset = position % 8U;

        unsigned int value =
            (code->bits[byte_index] >> offset) & 1U;

        if (bit_write_bit(writer, value) != 0)
            return -1;
    }

    return 0;
}


/* Copy a completed temporary bitstream into allocated memory. */
static int temporary_file_to_buffer(FILE *file, uint8_t **output, size_t *output_size){
    if (file == NULL || output == NULL || output_size == NULL)
        return -1;

    *output = NULL;
    *output_size = 0;

    if (fflush(file) != 0)
        return -1;

    if (fseek(file, 0, SEEK_END) != 0)
        return -1;

    long file_size = ftell(file);

    if (file_size < 0)
        return -1;

    if ((uintmax_t)file_size > SIZE_MAX)
        return -1;

    size_t size = (size_t)file_size;

    if (fseek(file, 0, SEEK_SET) != 0)
        return -1;

    if (size == 0)
        return 0;

    uint8_t *buffer = malloc(size);

    if (buffer == NULL)
        return -1;

    if (fread(buffer, 1, size, file) != size)
    {
        free(buffer);
        return -1;
    }

    *output = buffer;
    *output_size = size;

    return 0;
}


/* Compress data using RLE, LZ77 and canonical Huffman coding. */
int compressor_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t **compressed_data,
    size_t *compressed_size,
    CompressorMetadata *metadata,
    size_t max_chain_length){
    if (compressed_data == NULL || compressed_size == NULL || metadata == NULL || (input == NULL && input_size > 0) || max_chain_length == 0)
        return -1;

    *compressed_data = NULL;
    *compressed_size = 0;

    memset(metadata, 0, sizeof(*metadata));

    uint8_t *rle_data = NULL;
    size_t rle_size = 0;

    LZ77Token *tokens = NULL;
    size_t token_count = 0;

    uint8_t *serialized = NULL;
    size_t serialized_size = 0;

    uint64_t frequencies[HUFFMAN_SYMBOL_COUNT] = {0};

    uint16_t code_lengths[HUFFMAN_SYMBOL_COUNT] = {0};

    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT] = {0};

    HuffmanNode *root = NULL;

    FILE *temporary_file = NULL;

    int result = -1;

    /* Calculate original CRC32. */
    if (crc32_calculate(
            input, input_size, &metadata->crc32) != 0)
        goto cleanup;

    metadata->original_size = input_size;

    /* Stage 1: RLE compression. */
    if (rle_compress(
            input, input_size, &rle_data, &rle_size) != 0)
        goto cleanup;

    metadata->rle_size = rle_size;

    /* Stage 2: LZ77 compression. */
    if (lz77_compress(
            rle_data, rle_size,
            &tokens, &token_count,
            max_chain_length) != 0)
        goto cleanup;

    metadata->token_count = token_count;

    /* Stage 3: Token serialization. */
    if (token_codec_serialize(
            tokens, token_count,
            &serialized, &serialized_size) != 0)
        goto cleanup;

    metadata->serialized_size = serialized_size;

    /*
     * Empty input produces no serialized data.
     * Therefore, no Huffman processing is required.
     */
    if (serialized_size > 0)
    {
        /* Stage 4: Generate canonical Huffman codes. */
        if (huffman_count_frequencies(
                serialized, serialized_size, frequencies) != 0)
            goto cleanup;

        root = huffman_build_tree(frequencies);

        if (root == NULL)
            goto cleanup;

        if (huffman_calculate_code_lengths(
                root, code_lengths) != 0)
            goto cleanup;

        if (huffman_generate_canonical_codes(
                code_lengths, codes) != 0)
            goto cleanup;

        memcpy(
            metadata->huffman_code_lengths,
            code_lengths,
            sizeof(code_lengths));

        /* Stage 5: Encode the Huffman bitstream. */
        temporary_file = tmpfile();

        if (temporary_file == NULL)
            goto cleanup;

        BitWriter writer;

        if (bit_writer_init(&writer, temporary_file) != 0)
            goto cleanup;

        for (size_t i = 0; i < serialized_size; i++)
        {
            const HuffmanCode *code = &codes[serialized[i]];

            if (metadata->meaningful_bits >
                UINT64_MAX - code->length)
                goto cleanup;

            metadata->meaningful_bits += code->length;

            if (encode_symbol(&writer, code) != 0)
                goto cleanup;
        }

        if (bit_writer_flush(&writer) != 0)
            goto cleanup;

        if (temporary_file_to_buffer(
                temporary_file,
                compressed_data,
                compressed_size) != 0)
            goto cleanup;
    }

    result = 0;

cleanup:

    free(rle_data);
    free(tokens);
    free(serialized);

    huffman_destroy_tree(root);

    if (temporary_file != NULL)
        fclose(temporary_file);

    if (result != 0)
    {
        free(*compressed_data);
        *compressed_data = NULL;
        *compressed_size = 0;
        memset(metadata, 0, sizeof(*metadata));
    }

    return result;
}


/* Decompress data and verify its integrity. */
int compressor_decompress(
    const uint8_t *compressed_data,
    size_t compressed_size,
    const CompressorMetadata *metadata,
    uint8_t **output,
    size_t *output_size)
{
    if (output == NULL ||
        output_size == NULL ||
        metadata == NULL ||
        (compressed_data == NULL && compressed_size > 0))
        return -1;

    *output = NULL;
    *output_size = 0;

    if (metadata->original_size > SIZE_MAX ||
        metadata->rle_size > SIZE_MAX ||
        metadata->token_count > SIZE_MAX ||
        metadata->serialized_size > SIZE_MAX)
        return -1;

    if (metadata->meaningful_bits > UINT64_MAX - 7)
        return -1;

    uint64_t expected_compressed_size =
        (metadata->meaningful_bits + 7) / 8;

    if (expected_compressed_size != compressed_size)
        return -1;

    if (metadata->token_count > SIZE_MAX / LZ77_TOKEN_SERIALIZED_SIZE)
        return -1;

    if (metadata->serialized_size != metadata->token_count * LZ77_TOKEN_SERIALIZED_SIZE)
        return -1;

    uint8_t *decoded_serialized = NULL;

    LZ77Token *decoded_tokens = NULL;
    size_t decoded_token_count = 0;

    uint8_t *decoded_rle = NULL;
    size_t decoded_rle_size = 0;

    uint8_t *reconstructed = NULL;
    size_t reconstructed_size = 0;

    HuffmanCode codes[HUFFMAN_SYMBOL_COUNT] = {0};

    FILE *temporary_file = NULL;

    int result = -1;

    /* Empty input: no Huffman stream exists. */
    if (metadata->serialized_size == 0)
    {
        if (compressed_size != 0 ||
            metadata->meaningful_bits != 0 ||
            metadata->token_count != 0 ||
            metadata->rle_size != 0 ||
            metadata->original_size != 0)
            goto cleanup;
    }
    else
    {
        if (metadata->token_count == 0 ||
            metadata->meaningful_bits == 0 ||
            compressed_size == 0)
            goto cleanup;

        /* Reconstruct canonical Huffman codes. */
        for (size_t i = 0; i < HUFFMAN_SYMBOL_COUNT; i++){
            if (metadata->huffman_code_lengths[i] > 256)
                goto cleanup;

        }

        if (huffman_generate_canonical_codes(metadata->huffman_code_lengths, codes) != 0)
            goto cleanup;

        /* Prepare the compressed bitstream for reading. */
        temporary_file = tmpfile();

        if (temporary_file == NULL)
            goto cleanup;

        if (fwrite(
                compressed_data, 1, compressed_size,
                temporary_file) != compressed_size)
            goto cleanup;

        if (fseek(temporary_file, 0, SEEK_SET) != 0)
            goto cleanup;

        BitReader reader;

        if (bit_reader_init(&reader, temporary_file) != 0)
            goto cleanup;

        size_t serialized_size =
            (size_t)metadata->serialized_size;

        decoded_serialized = malloc(serialized_size);

        if (decoded_serialized == NULL)
            goto cleanup;

        /* Decode exactly the recorded number of symbols. */
        for (size_t i = 0; i < serialized_size; i++)
        {
            if (huffman_decode_symbol(
                    &reader, codes, &decoded_serialized[i]) != 0)
                goto cleanup;
        }
    }

    /* Stage 1: Deserialize LZ77 tokens. */
    if (token_codec_deserialize(
            decoded_serialized,
            (size_t)metadata->serialized_size,
            &decoded_tokens,
            &decoded_token_count) != 0)
        goto cleanup;

    if (decoded_token_count != metadata->token_count)
        goto cleanup;

    /* Stage 2: LZ77 decompression. */
    if (lz77_decompress(
            decoded_tokens,
            decoded_token_count,
            &decoded_rle,
            &decoded_rle_size) != 0)
        goto cleanup;

    if (decoded_rle_size != metadata->rle_size)
        goto cleanup;

    /* Stage 3: RLE decompression. */
    if (rle_decompress(
            decoded_rle,
            decoded_rle_size,
            &reconstructed,
            &reconstructed_size) != 0)
        goto cleanup;

    if (reconstructed_size != metadata->original_size)
        goto cleanup;

    /* Stage 4: Verify CRC32. */
    uint32_t reconstructed_crc = 0;

    if (crc32_calculate(
            reconstructed,
            reconstructed_size,
            &reconstructed_crc) != 0)
        goto cleanup;

    if (reconstructed_crc != metadata->crc32)
        goto cleanup;

    *output = reconstructed;
    *output_size = reconstructed_size;

    reconstructed = NULL;

    result = 0;

cleanup:

    free(decoded_serialized);
    free(decoded_tokens);
    free(decoded_rle);
    free(reconstructed);

    if (temporary_file != NULL)
        fclose(temporary_file);

    return result;
}