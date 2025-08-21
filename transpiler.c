#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    // Check if not enough args provided or
    // the help flag is used
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {

        // Display error and exit
        printf("[zirc] USAGE: zirc <input.zir> <output.c>\n");
        exit(1);

    }

    FILE *input_file = fopen(argv[1], "r");  // Open the input file

    // Check if file is not found
    if (input_file == NULL) {

        // Display error and exit
        printf("[zirc] ERROR: INPUT FILE '%s' NOT FOUND\n", argv[1]);
        exit(1);

    }

    unsigned int max_line_size = 256;
    char line_buffer[max_line_size];

    // Print each line of the file until EOF reached
    while (fgets(line_buffer, max_line_size, input_file)) {

            printf("%s", line_buffer);

    }

    fclose(input_file);  // Close the input file

    return 0;

}
