#include <stdio.h>
#include "file_compression.h"

int main(){

    printf("\n====================================\n");
    printf(" FOLDER COMPRESSION TEST\n");
    printf("====================================\n");

    if (compress_directory_to_archive("test_data/folder_sample", "test_data/folder_sample.cmp", 64) != 0){
        printf("\nFAIL: Folder compression.\n");
        return 1;
    }

    printf("\nPASS: Folder compression completed.\n");

    return 0;
}
