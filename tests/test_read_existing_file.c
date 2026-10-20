#include "file_io.h"
#include <stdio.h>

int main(){
char filename[1024];

printf("Enter the path of an existing file: ");

if (fgets(filename, sizeof(filename), stdin) == NULL){
    printf("Error: Could not read the file path.\n");
    return 1;
}

for (int i = 0; filename[i] != '\0'; i++){
    if (filename[i] == '\n'){
        filename[i] = '\0';
        break;
    }
}

FileBuffer buffer = {NULL, 0};

if (read_file(filename, &buffer) != 0){
    printf("Error: Could not open or read the file.\n");
    return 1;
}

printf("\nFile read successfully!\n");
printf("File path: %s\n", filename);
printf("File size: %zu bytes\n", buffer.size);

free_file_buffer(&buffer);

printf("Memory released successfully.\n");

return 0;

}
