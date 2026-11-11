#include <stdio.h>
#include "file_compression.h"

int main(){

    printf("\n====================================\n");
    printf(" MULTIPLE FILE DECOMPRESSION TEST\n");
    printf("====================================\n");

    if (decompress_archive_to_directory("test_data/multiple.cmp", "test_data/extracted_multi") != 0){
        printf("\nFAIL: Multiple-file decompression.\n");
        return 1;
    }

    printf("\nPASS: Multiple-file decompression completed.\n");

    return 0;
}
