#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    // Check if not enough args are provided
    if (argc < 2) {

        fprintf(stderr, "[zirc] ERROR: INSUFFICIENT ARGUMENTS PROVIDED\n");
        exit(1);

    }

    // Check if help flag is used
    if (strcmp(argv[1], "--help") == 0) {

        fprintf(stderr, "[zirc] USAGE: zirc <input.zir> <output.c>\n");
        exit(0);

    }

    FILE *input_file = fopen(argv[1], "r");  // Open the input file

    // Check if file is not found
    if (input_file == NULL) {

        fprintf(stderr, "[zirc] ERROR: INPUT FILE '%s' NOT FOUND\n", argv[1]);
        exit(1);

    }

    // Figure out the file size by moving the file ptr to the end
    fseek(input_file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(input_file);
    rewind(input_file); // Return file ptr to start

    // Allocate buffer large enough for the file + null terminator
    char *file_content = malloc(file_size + 1);

    if (!file_content) {

        fprintf(stderr, "[zirc] ERROR: OUT OF MEMORY\n");
        free(file_content);
        fclose(input_file);

        exit(1);

    }

    // Read the whole file into the buffer
    fread(file_content, sizeof(char), file_size, input_file);
    file_content[file_size] = '\0';  // Append null terminator

    fclose(input_file);

    printf("%s\n", file_content);  // Display the file content

    free(file_content);

    return 0;

}
