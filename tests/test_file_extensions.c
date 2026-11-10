#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "file_io.h"
#include "file_compression.h"

#define TEST_COUNT 7

typedef struct {
    const char *extension;
    const uint8_t *data;
    size_t size;
} TestFile;

static void put16(uint8_t *p, uint16_t v){
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v){
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

int main(){

    uint8_t dat[512];
    uint8_t bin[512];

    /* Include every possible byte value. */
    for (int i = 0; i < 512; i++) {
        dat[i] = (uint8_t)i;
        bin[i] = (uint8_t)(255 - (i % 256));
    }

    /* Valid 2x2, 24-bit BMP image. */
    uint8_t bmp[70] = {0};

    bmp[0] = 'B';
    bmp[1] = 'M';
    put32(bmp + 2, 70);
    put32(bmp + 10, 54);
    put32(bmp + 14, 40);
    put32(bmp + 18, 2);
    put32(bmp + 22, 2);
    put16(bmp + 26, 1);
    put16(bmp + 28, 24);
    put32(bmp + 34, 16);

    /* Two rows of two pixels, padded to 4-byte boundaries. */
    for (int i = 54; i < 70; i++)
        bmp[i] = (uint8_t)(i * 7);

    /* Valid PCM WAV: mono, 8000 Hz, 8-bit, 8000 samples. */
    uint8_t wav[8044] = {0};

    memcpy(wav, "RIFF", 4);
    put32(wav + 4, 8036);
    memcpy(wav + 8, "WAVE", 4);
    memcpy(wav + 12, "fmt ", 4);
    put32(wav + 16, 16);
    put16(wav + 20, 1);
    put16(wav + 22, 1);
    put32(wav + 24, 8000);
    put32(wav + 28, 8000);
    put16(wav + 32, 1);
    put16(wav + 34, 8);
    memcpy(wav + 36, "data", 4);
    put32(wav + 40, 8000);

    for (int i = 44; i < 8044; i++)
        wav[i] = (uint8_t)(128 + (i % 16));

    static const uint8_t txt[] =
        "This is a text file used for compression testing. "
        "Lossless compression must restore every original byte. "
        "This sentence is repeated to test compression efficiency. "
        "This sentence is repeated to test compression efficiency.";

    static const uint8_t csv[] =
        "ID,Name,Department,CGPA\n"
        "101,Rafi,CSE,3.85\n"
        "102,Hasan,EEE,3.75\n"
        "103,Karim,CSE,3.90\n"
        "104,Nadia,EEE,3.80\n";

    static const uint8_t log[] =
        "[2026-11-10 10:00:00] INFO Application started\n"
        "[2026-11-10 10:00:01] INFO Reading input file\n"
        "[2026-11-10 10:00:02] WARN Compression level selected\n"
        "[2026-11-10 10:00:03] INFO Compression completed\n"
        "[2026-11-10 10:00:04] INFO Application finished\n";

    TestFile files[TEST_COUNT] = {
        {"txt", txt, sizeof(txt) - 1},
        {"csv", csv, sizeof(csv) - 1},
        {"log", log, sizeof(log) - 1},
        {"dat", dat, sizeof(dat)},
        {"bin", bin, sizeof(bin)},
        {"bmp", bmp, sizeof(bmp)},
        {"wav", wav, sizeof(wav)}
    };

    int passed = 0;

    printf("\n====================================\n");
    printf("     FILE EXTENSION TESTING\n");
    printf("====================================\n\n");

    for (int i = 0; i < TEST_COUNT; i++)
    {
        char original_path[100];
        char archive_path[100];
        char restored_path[100];

        snprintf(original_path, sizeof(original_path),
                 "test_data/extension_test.%s", files[i].extension);

        snprintf(archive_path, sizeof(archive_path),
                 "test_data/extension_test_%s.cmp", files[i].extension);

        snprintf(restored_path, sizeof(restored_path),
                 "test_data/restored_%s.%s",
                 files[i].extension, files[i].extension);

        printf("Testing .%s ... ", files[i].extension);

        if (write_file(original_path, files[i].data,
                       files[i].size) != 0)
        {
            printf("FAIL: File creation\n");
            continue;
        }

        if (compress_file_to_archive(original_path,
                                     archive_path, 64) != 0)
        {
            printf("FAIL: Compression\n");
            continue;
        }

        if (decompress_archive_to_file(archive_path,
                                       restored_path) != 0)
        {
            printf("FAIL: Decompression\n");
            continue;
        }

        FileBuffer restored = {NULL, 0};

        if (read_file(restored_path, &restored) != 0)
        {
            printf("FAIL: Reading restored file\n");
            continue;
        }

        int matches =
            restored.size == files[i].size &&
            memcmp(restored.data, files[i].data,
                   files[i].size) == 0;

        free_file_buffer(&restored);

        if (matches)
        {
            printf("PASS: Byte-for-byte match\n");
            passed++;
        }
        else
        {
            printf("FAIL: Data mismatch\n");
        }
    }

    printf("\n====================================\n");
    printf("RESULT: %d/%d FILE TYPES PASSED\n",
           passed, TEST_COUNT);
    printf("====================================\n");

    return passed == TEST_COUNT ? 0 : 1;
}
