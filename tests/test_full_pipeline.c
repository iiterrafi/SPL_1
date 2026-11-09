#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rle.h"
#include "lz77.h"
#include "token_codec.h"
#include "huffman.h"
#include "bitstream.h"
#include "crc32.h"

/* Encode one byte using its canonical Huffman code. */
static int encode_symbol(BitWriter *writer, const HuffmanCode *code){
    for (unsigned int bit = 0; bit < code->length; bit++){
        unsigned int position = code->length - 1U - bit;
        unsigned int byte_index = 31U - position / 8U;
        unsigned int offset = position % 8U;

        unsigned int value = (code->bits[byte_index] >> offset) & 1U;

        if (bit_write_bit(writer, value) != 0)
            return -1;
    }

    return 0;
}

static int run_pipeline_test(const char *test_name, const uint8_t *original, size_t original_size){
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

    uint8_t *decoded_serialized = NULL;
    LZ77Token *decoded_tokens = NULL;
    size_t decoded_token_count = 0;

    uint8_t *decoded_rle = NULL;
    size_t decoded_rle_size = 0;

    uint8_t *reconstructed = NULL;
    size_t reconstructed_size = 0;

    uint32_t original_crc = 0;
    uint32_t reconstructed_crc = 0;

    FILE *temporary_file = NULL;

    int result = -1;

    printf("\n--- %s ---\n", test_name);

    /* Calculate original checksum. */
    if (crc32_calculate(original, original_size, &original_crc) != 0)
        goto cleanup;

    /* Stage 1: RLE compression. */
    if (rle_compress(original, original_size, &rle_data, &rle_size) != 0)
        goto cleanup;

    printf("PASS: RLE compression (%zu bytes).\n", rle_size);

    /* Stage 2: LZ77 compression. */
    if (lz77_compress(rle_data, rle_size, &tokens, &token_count, 16) != 0)
        goto cleanup;

    printf("PASS: LZ77 compression (%zu tokens).\n", token_count);

    /* Stage 3: Serialize LZ77 tokens. */
    if (token_codec_serialize(tokens, token_count, &serialized, &serialized_size) != 0)
        goto cleanup;

    printf("PASS: Token serialization (%zu bytes).\n", serialized_size);

    /*
     * Empty input produces no serialized bytes.
     * Huffman processing is therefore skipped.
     */
    if (serialized_size > 0){
        /* Stage 4: Build canonical Huffman codes. */
        if (huffman_count_frequencies(
                serialized, serialized_size, frequencies) != 0)
            goto cleanup;

        root = huffman_build_tree(frequencies);

        if (root == NULL)
            goto cleanup;

        if (huffman_calculate_code_lengths(root, code_lengths) != 0)
            goto cleanup;

        if (huffman_generate_canonical_codes(
                code_lengths, codes) != 0)
            goto cleanup;

        printf("PASS: Huffman code generation.\n");

        /* Stage 5: Encode Huffman data into a bitstream. */
        temporary_file = tmpfile();

        if (temporary_file == NULL)
            goto cleanup;

        BitWriter writer;

        if (bit_writer_init(&writer, temporary_file) != 0)
            goto cleanup;

        for (size_t i = 0; i < serialized_size; i++)
        {
            if (encode_symbol(&writer, &codes[serialized[i]]) != 0)
                goto cleanup;
        }

        if (bit_writer_flush(&writer) != 0)
            goto cleanup;

        printf("PASS: Huffman bitstream encoding.\n");

        /* Prepare to decode the bitstream. */
        rewind(temporary_file);

        BitReader reader;

        if (bit_reader_init(&reader, temporary_file) != 0)
            goto cleanup;

        decoded_serialized = malloc(serialized_size);

        if (decoded_serialized == NULL)
            goto cleanup;

        /* Decode exactly the original number of serialized bytes. */
        for (size_t i = 0; i < serialized_size; i++)
        {
            if (huffman_decode_symbol(
                    &reader, codes, &decoded_serialized[i]) != 0)
                goto cleanup;
        }

        printf("PASS: Huffman bitstream decoding.\n");

        if (memcmp(serialized, decoded_serialized,
                   serialized_size) != 0)
            goto cleanup;

        printf("PASS: Serialized data matches after Huffman decoding.\n");
    }
    else
    {
        printf("PASS: Empty Huffman stream handled.\n");
    }

    /* Reverse Stage 3: Deserialize LZ77 tokens. */
    if (token_codec_deserialize(
            decoded_serialized, serialized_size,
            &decoded_tokens, &decoded_token_count) != 0)
        goto cleanup;

    printf("PASS: Token deserialization.\n");

    /* Reverse Stage 2: LZ77 decompression. */
    if (lz77_decompress(
            decoded_tokens, decoded_token_count,
            &decoded_rle, &decoded_rle_size) != 0)
        goto cleanup;

    printf("PASS: LZ77 decompression.\n");

    /* Reverse Stage 1: RLE decompression. */
    if (rle_decompress(
            decoded_rle, decoded_rle_size,
            &reconstructed, &reconstructed_size) != 0)
        goto cleanup;

    printf("PASS: RLE decompression.\n");

    /* Verify the original data. */
    if (reconstructed_size != original_size)
        goto cleanup;

    if (original_size > 0 &&
        memcmp(original, reconstructed, original_size) != 0)
        goto cleanup;

    printf("PASS: Original and reconstructed data match.\n");

    /* Verify CRC32. */
    if (crc32_calculate(
            reconstructed, reconstructed_size,
            &reconstructed_crc) != 0)
        goto cleanup;

    if (original_crc != reconstructed_crc)
        goto cleanup;

    printf("PASS: CRC32 verification (0x%08X).\n", original_crc);

    printf("Original size: %zu bytes\n", original_size);

    result = 0;

cleanup:

    if (result != 0)
        printf("FAIL: %s pipeline test.\n", test_name);

    free(rle_data);
    free(tokens);
    free(serialized);
    free(decoded_serialized);
    free(decoded_tokens);
    free(decoded_rle);
    free(reconstructed);

    huffman_destroy_tree(root);

    if (temporary_file != NULL)
        fclose(temporary_file);

    return result;
}

int main(void)
{
    const uint8_t text_data[] =
        "This is a test of our complete lossless compression pipeline. "
        "This is a test of our complete lossless compression pipeline.";

    const uint8_t repetitive_data[] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    const uint8_t binary_data[] = {
        0x00, 0xFF, 0x80, 0x00, 0x7F, 0xFF,
        0x01, 0x02, 0x03, 0x00, 0x80, 0xFF,
        0x10, 0x10, 0x10, 0x10
    };

    printf("========================================\n");
    printf(" COMPLETE COMPRESSION PIPELINE TEST\n");
    printf("========================================\n");

    if (run_pipeline_test(
            "Ordinary Text",
            text_data, sizeof(text_data) - 1) != 0)
        return 1;

    if (run_pipeline_test(
            "Repetitive Data",
            repetitive_data, sizeof(repetitive_data) - 1) != 0)
        return 1;

    if (run_pipeline_test(
            "Binary Data",
            binary_data, sizeof(binary_data)) != 0)
        return 1;

    if (run_pipeline_test(
            "Empty Input",
            NULL, 0) != 0)
        return 1;

    printf("\n========================================\n");
    printf(" ALL PIPELINE TESTS PASSED!\n");
    printf("========================================\n");

    return 0;
}
