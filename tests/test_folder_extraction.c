#include <stdio.h>
#include "file_compression.h"

int main(){

    printf("\n====================================\n");
    printf(" FOLDER EXTRACTION TEST\n");
    printf("====================================\n");

    if (decompress_archive_to_directory("test_data/folder_sample.cmp", "test_data/folder_extracted") != 0){
        printf("\nFAIL: Folder extraction.\n");
        return 1;
    }

    printf("\nPASS: Folder extraction completed.\n");

    return 0;
}
