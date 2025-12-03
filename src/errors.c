#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "colours.h"
#include "errors.h"

// Internal errors
void error_oom(void) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Out of memory.\n", FG_RED_BOLD, ERR_OOM, RESET);
    fprintf(stderr, "%s┃ Help:%s Reduce memory usage or increase its capacity.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_open_str(int line) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Unterminated string (line %d).\n", FG_RED_BOLD, ERR_OPEN_STR, RESET, line);
    fprintf(stderr, "%s┃ Help:%s Use a closing '\"' to end a string.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_expect_symbol(int line, const char *expected) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Expected %s (line %d).\n", FG_RED_BOLD, ERR_EXPECT_SYMBOL, RESET, expected, line);
    fprintf(stderr, "%s┃ Help:%s Implement the missing symbol.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_expect_expression(int line) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Expected expression (line %d).\n", FG_RED_BOLD, ERR_EXPECT_EXPRESSION, RESET, line);
    fprintf(stderr, "%s┃ Help:%s Implement an expression after a statement.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_expect_statement(int line) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Expected statement or declaration (line %d).\n", FG_RED_BOLD, ERR_EXPECT_STATEMENT, RESET, line);
    fprintf(stderr, "%s┃ Help:%s Implement a statement or declaration.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_invalid_token(int line) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Unexpected token at global scope (line %d).\n", FG_RED_BOLD, ERR_INVALID_TOK, RESET, line);
    fprintf(stderr, "%s┃ Help:%s Remove the invalid symbol; only 'entry' blocks are valid in global scope.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_multiple_entry(void) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Multiple program entrypoints found.\n", FG_RED_BOLD, ERR_MANY_ENTRY, RESET);
    fprintf(stderr, "%s┃ Help:%s Use only 1 'entry' block.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_no_entry(void) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s No program entrypoint found.\n", FG_RED_BOLD, ERR_NO_ENTRY, RESET);
    fprintf(stderr, "%s┃ Help:%s You must have an 'entry' block to signify program entrypoint.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_type_mismatch(const char *var_name, const char *expected_type, const char *actual_type) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Type mismatch for variable '%s'.\n", FG_RED_BOLD, ERR_TYPE_MISMATCH, RESET, var_name);
    fprintf(stderr, "%s┃ Help:%s Use %s instead of the current %s.\n", FG_BLUE_BOLD, RESET, expected_type, actual_type);
    exit(1);

}

void error_invalid_opcode(int op) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Opcode '%d' is unknown.\n", FG_RED_BOLD, ERR_INVALID_OPCODE, RESET, op);
    fprintf(stderr, "%s┃ Help:%s Unavailable (INTERNAL ERROR).\n", FG_PURPLE_BOLD, RESET);
    exit(1);

}

void error_invalid_var_index(size_t var_count) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Invalid variable index.\n", FG_RED_BOLD, ERR_INVALID_VAR_INDEX, RESET);
    fprintf(stderr, "%s┃ Help:%s Reduce total amount of variables; only up to %zu variables are allowed.\n", FG_BLUE_BOLD, RESET, var_count);
    exit(1);

}

void error_invalid_const_index(size_t const_count) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Invalid constant index.\n", FG_RED_BOLD, ERR_INVALID_CONST_INDEX, RESET);
    fprintf(stderr, "%s┃ Help:%s Reduce total amount of constants; only up to %zu constants are allowed.\n", FG_BLUE_BOLD, RESET, const_count);
    exit(1);

}

void error_vm_oob(void) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s VM pointer out of bounds.\n", FG_RED_BOLD, ERR_VM_POS_OOB, RESET);
    fprintf(stderr, "%s┃ Help:%s Unavailable (INTERNAL ERROR).\n", FG_PURPLE_BOLD, RESET);
    exit(1);

}

void error_wrong_var_init(size_t var_count, size_t init_count) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Variable initialization mismatch.\n", FG_RED_BOLD, ERR_WRONG_VAR_INIT, RESET);
    fprintf(stderr, "%s┃ Help:%s %zu variables declared but %zu initializers exist.\n", FG_BLUE_BOLD, RESET, var_count, init_count);
    exit(1);

}

void error_undefined_var(const char *name) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Variable '%s' is undefined.\n", FG_RED_BOLD, ERR_UNDEFINED_VAR, RESET, name);
    fprintf(stderr, "%s┃ Help:%s Variables must be declared before use.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

// CLI errors
void error_no_args(void) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Insufficient arguments.\n", FG_RED_BOLD, ERR_NO_ARGS, RESET);
    fprintf(stderr, "%s┃ Help:%s At least 1 argument is required (<input_file.phase>).\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_invalid_arg(const char *arg) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Argument '%s' is invalid.\n", FG_RED_BOLD, ERR_INVALID_ARG, RESET, arg);
    fprintf(stderr, "%s┃ Help:%s See all available arguments with 'phase --help'.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

void error_ifnf(const char *name) {

    fprintf(stderr, "%s┃ ERROR [%d]:%s Input file '%s' not found.\n", FG_RED_BOLD, ERR_NO_INPUT, RESET, name);
    fprintf(stderr, "%s┃ Help:%s Use a valid input path (e.g. /path/to/file.phase).\n", FG_BLUE_BOLD, RESET);
    exit(1);

}
