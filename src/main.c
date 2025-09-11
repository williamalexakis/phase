#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "codegen.c"

static void indent(int n) { for (int i = 0; i < n; i++) putchar(' '); }

static void print_expression(AstExpression *expression, int ind) {

    switch (expression->tag) {

        case E_STRING:

            indent(ind);
            printf("╰ EXPRESSION (%sSTRING%s) [%s\"%s\"%s]\n", FG_CYAN, RESET, FG_PURPLE, expression->str_lit.value, RESET);
            break;

    }

}

static void print_statement(AstStatement *statement, int ind) {

    switch (statement->tag) {

        case S_OUT:

            indent(ind);
            printf("╰ STATEMENT (%sOUT%s)\n", FG_CYAN, RESET);
            print_expression(statement->out.expression, ind + 8);
            break;

    }

}

static void print_block(AstBlock *block, int ind) {

    indent(ind);
    printf("╰ BLOCK ╮\n");

    for (size_t i = 0; i < block->len; i++) print_statement(block->statements[i], ind + 8);

}

static void print_declaration(AstDeclare *declare, int ind) {

    switch (declare->tag) {

        case D_ENTRY:

            indent(ind);
            printf("╰ DECLARATION (%sENTRY%s)\n", FG_CYAN, RESET);
            print_block(declare->entry.block, ind + 8);
            break;

    }

}

static void print_program(AstProgram *program) {

    printf("PROGRAM ╮\n");

    for (size_t i = 0; i < program->len; i++) print_declaration(program->decls[i], 8);

    exit(0);

}

static void display_tokens(Lexer *lexer) {

    const char *token_names[] = {

        "EOF",
        "NEWLINE",
        "LBRACE",
        "RBRACE",
        "ENTRY",
        "OUT",
        "STRING",
        "UNKNOWN"

    };

    // Display the tokenized form of the source
    // file, including the src lexemes and line
    // numbers
    for (;;) {

        Token token = next_token(lexer);

        printf("%d | ", token.line);                 // Display line num
        printf("%s%s%s", FG_CYAN, token_names[token.type], RESET);              // Display token type
        if (token.lexeme) printf(" %s'%s'%s", FG_PURPLE, token.lexeme, RESET);  // Display the lexeme
        printf("\n");

        // Use a linear search to determine if
        // the token has an allocated lexeme
        const TokenType heap_tokens[3] = {T_OUT, T_ENTRY, T_STRING};
        const size_t ht_len = sizeof(heap_tokens) / sizeof(heap_tokens[0]);
        const bool needs_freeing = is_heap_lexeme(token, heap_tokens, ht_len);

        // Free the lexeme only if it was allocated memory,
        // in this case only if the token is an ident, kw,
        // or string
        if (token.lexeme && needs_freeing) {

            free(token.lexeme);

        }

        if (token.type == T_EOF) break;

    }

    exit(0);

}

static void help_flag() {

    printf("USAGE: %s./phasec <input.phase> <output.c / --output-flag> ...%s\n\n", FG_GREEN, RESET);
    printf("Flags:\n\t%s--tokens%s\tDisplays the source file as tokens.\n", FG_GREEN, RESET);
    exit(0);

}

int main(int argc, char **argv) {

    bool token_mode = false;
    bool ast_mode = false;
    char *valid_args[] = {"--help", "--tokens", "--ast"};
    FILE *output_file = NULL;

    // Check if not enough args are provided
    if (argc < 3) {

        fprintf(stderr, "[phasec] ERROR: INSUFFICIENT ARGUMENTS PROVIDED\n");
        exit(1);

    }

    FILE *input_file = fopen(argv[1], "r");  // Open the input file

    // Check if file is not found
    if (!input_file) {

        fprintf(stderr, "[phasec] ERROR: INPUT FILE '%s' NOT FOUND\n", argv[1]);
        exit(1);

    }

    // Figure out the file size by moving the file ptr to the end
    fseek(input_file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(input_file);
    rewind(input_file); // Return file ptr to start

    // Allocate buffer large enough for the file + null terminator
    char *file_content = malloc(file_size + 1);

    if (!file_content) {

        fprintf(stderr, "[phasec] ERROR: OUT OF MEMORY\n");
        free(file_content);
        fclose(input_file);

        exit(1);

    }

    // Read the whole file into the buffer
    fread(file_content, sizeof(char), file_size, input_file);
    file_content[file_size] = '\0';  // Append null terminator for str ops

    fclose(input_file);

    for (int i = 2; i < argc; i++) {

        if (strcmp(argv[i], "--help") == 0) {

            help_flag();

        } else if (strcmp(argv[i], "--tokens") == 0) {

            token_mode = true;

        } else if (strcmp(argv[i], "--ast") == 0) {

            ast_mode = true;

        } else {

            fprintf(stderr, "ERROR: UNRECOGNIZED ARGUMENT '%s'\n", argv[i]);
            exit(1);

        }
    }

    if (!token_mode && !ast_mode) output_file = fopen(argv[1], "w");

    if (!output_file && !token_mode && !ast_mode) {

        error_no_output();

    }

    // Initialize lexer
    Lexer lexer = { .src = file_content, .pos = 0, .line = 1 };

    if (token_mode) display_tokens(&lexer);

    Parser parser = init_parser(&lexer);
    AstProgram *program = parse_program(&parser);

    if (ast_mode) print_program(program);

    Emitter emitter = { .output = output_file, .indent = 0 };

    emit_program(&emitter, program);

    fclose(output_file);

    free_program(program);
    free_token(&parser.look);
    free(file_content);

    printf("%s%s[phasec] PROGRAM BUILT%s\n", FG_BLUE, BG_GREEN, RESET);

    return 0;

}
