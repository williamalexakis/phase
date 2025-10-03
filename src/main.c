#include "codegen.c"
#include "colours.h"

static void indent(int n) { for (int i = 0; i < n; i++) putchar(' '); }

static void print_expression(AstExpression *expression, int ind) {

    switch (expression->tag) {

        case EXP_STRING: {

            indent(ind);
            printf("╰ EXPRESSION (%sSTRING%s) [%s\"%s\"%s]\n", FG_CYAN, RESET, FG_PURPLE, expression->str_lit.value, RESET);

        } break;

        case EXP_INTEGER: {

            indent(ind);
            printf("╰ EXPRESSION (%sINTEGER%s) [%s%d%s]\n", FG_CYAN, RESET, FG_PURPLE, expression->int_lit.value, RESET);

        } break;

        case EXP_FLOAT: {

            indent(ind);
            printf("╰ EXPRESSION (%sFLOAT%s) [%s%g%s]\n", FG_CYAN, RESET, FG_PURPLE, expression->float_lit.value, RESET);

        } break;

        case EXP_BOOLEAN: {

            indent(ind);
            printf("╰ EXPRESSION (%sBOOLEAN%s) [%s%s%s]\n", FG_CYAN, RESET, FG_PURPLE, expression->bool_lit.value ? "true" : "false", RESET);

        } break;

        case EXP_VARIABLE: {

            indent(ind);
            printf("╰ EXPRESSION (%sVARIABLE%s) [%s%s%s]\n", FG_CYAN, RESET, FG_PURPLE, expression->variable.name, RESET);

        } break;

    }

}

static void print_statement(AstStatement *statement, int ind) {

    switch (statement->tag) {

        case STM_OUT: {

            indent(ind);
            printf("╰ STATEMENT (%sOUT%s)\n", FG_CYAN, RESET);
            print_expression(statement->out.expression, ind + 6);

        } break;

        case STM_ASSIGN: {

            indent(ind);
            printf("╰ STATEMENT (%sASSIGNMENT%s) [%s%s%s]\n", FG_CYAN, RESET, FG_PURPLE, statement->assign.var_name, RESET);
            print_expression(statement->assign.expression, ind + 6);

        } break;

        case STM_VAR_DECL: {

            indent(ind);
            printf("╰ STATEMENT (%sVAR DECLARATION%s) [%s%s%s", FG_CYAN, RESET,
                   FG_PURPLE, token_type_to_string(statement->var_decl.var_type), RESET);

            for (size_t i = 0; i < statement->var_decl.var_count; i++) {

                if (i == 0) {

                    printf(" ");

                } else {

                    printf(", ");

                }

                printf("%s%s%s", FG_PURPLE, statement->var_decl.var_names[i], RESET);

            }

            printf("]\n");

            for (size_t i = 0; i < statement->var_decl.init_count; i++) {

                print_expression(statement->var_decl.init_exprs[i], ind + 6);

            }

        } break;

    }

}

static void print_block(AstBlock *block, int ind) {

    indent(ind);
    printf("╰ BLOCK\n");

    for (size_t i = 0; i < block->len; i++) print_statement(block->statements[i], ind + 6);

}

static void print_declaration(AstDeclaration *declare, int ind) {

    switch (declare->tag) {

        case DEC_ENTRY: {

            indent(ind);
            printf("╰ DECLARATION (%sENTRY%s)\n", FG_CYAN, RESET);
            print_block(declare->entry.block, ind + 6);

        } break;

        case DEC_VAR: {

            indent(ind);
            printf("╰ DECLARATION (%sVAR%s) [%s%s%s", FG_CYAN, RESET,
                   FG_PURPLE, token_type_to_string(declare->var_decl.var_type), RESET);

            if (declare->var_decl.var_count == 0) {

                printf(" (anonymous)");

            } else {

                for (size_t i = 0; i < declare->var_decl.var_count; i++) {

                    if (i == 0) {

                        printf(" ");

                    } else {

                        printf(", ");

                    }

                    printf("%s%s%s", FG_PURPLE, declare->var_decl.var_names[i], RESET);

                }

            }

            printf("]\n");

        } break;

    }

}

static void print_program(AstProgram *program) {

    printf("PROGRAM\n");

    for (size_t i = 0; i < program->len; i++) print_declaration(program->declarations[i], 6);

    exit(0);

}

static void display_tokens(Lexer *lexer) {

    for (;;) {

        Token token = next_token(lexer);

        printf("%d | ", token.line);  // Display line num
        printf("%s%s%s", FG_CYAN, get_token_name(token.type), RESET);  // Display token type

        if (token.lexeme) printf(" %s'%s'%s", FG_PURPLE, token.lexeme, RESET);  // Display the lexeme

        printf("\n");

        if (token.lexeme && token.heap_allocated) free(token.lexeme);
        if (token.type == TOK_EOF) break;

    }

    exit(0);

}

static void help_flag() {

    printf("Usage: %s./phase <input.phase>%s\n\n", FG_BLUE_BOLD, RESET);
    printf("Flags:\n");
    printf("\t%s--help%s : Display usage information (input file not required)\n", FG_BLUE_BOLD, RESET);
    printf("\t%s--tokens%s : Display the source file as tokens\n", FG_BLUE_BOLD, RESET);
    printf("\t%s--ast%s : Display the source file as an AST\n", FG_BLUE_BOLD, RESET);
    printf("\t%s--loud%s : Display a message upon program completion\n", FG_BLUE_BOLD, RESET);

    exit(0);

}

int main(int argc, char **argv) {

    bool token_mode = false;
    bool ast_mode = false;
    bool loud_mode = false;

    if (argc < 2) error_no_args();
    if (strcmp(argv[1], "--help") == 0) help_flag();

    FILE *input_file = fopen(argv[1], "r");  // Open the input file

    if (!input_file) error_ifnf(argv[1]);

    // Move file ptr to end to determine size
    fseek(input_file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(input_file);
    rewind(input_file);  // Return to start

    char *file_content = malloc(file_size + 1);

    if (!file_content) {

        fclose(input_file);
        error_oom();

    }

    fread(file_content, sizeof(char), file_size, input_file);

    file_content[file_size] = '\0';  // Null terminate the str

    fclose(input_file);  // Close input file

    for (int i = 2; i < argc; i++) {

        if (strcmp(argv[i], "--help") == 0) {

            help_flag();

        } else if (strcmp(argv[i], "--tokens") == 0) {

            token_mode = true;

        } else if (strcmp(argv[i], "--ast") == 0) {

            ast_mode = true;

        } else if (strcmp(argv[i], "--loud") == 0) {

            loud_mode = true;

        } else {

            error_invalid_arg(argv[i]);

        }

    }

    Lexer lexer = { .src = file_content, .pos = 0, .line = 1 };

    if (token_mode) {

        display_tokens(&lexer);
        free(file_content);

    }

    Parser parser = init_parser(&lexer);
    AstProgram *program = parse_program(&parser);

    if (ast_mode) {

        print_program(program);
        free_program(program);
        free_token(&parser.look);
        free(file_content);

    }

    if (!token_mode && !ast_mode) {

        Emitter emitter = {0};
        emit_program(&emitter, program);

        VM vm = {0};
        init_vm(&vm, emitter.constants, emitter.const_count, emitter.code, emitter.code_len);

        interpret(&vm);

        free_vm(&vm);
        free_emitter(&emitter);
        free_program(program);
        free_token(&parser.look);
        free(file_content);

        if (loud_mode) printf("\n%sPROGRAM EXECUTED%s\n", FG_GREEN_BOLD, RESET);

    }

    return 0;

}
