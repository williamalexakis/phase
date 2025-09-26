#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "colours.h"

typedef enum {

    // Internal errors
    ERR_OOM = 100,
    ERR_OPEN_STR,
    ERR_EXPECT_SYMBOL,
    ERR_EXPECT_EXPRESSION,
    ERR_EXPECT_STATEMENT,
    ERR_INVALID_TOK,
    ERR_MANY_ENTRY,
    ERR_NO_ENTRY,
    ERR_TYPE_MISMATCH,
    ERR_INVALID_OPCODE,
    ERR_INVALID_VAR_INDEX,
    ERR_INVALID_CONST_INDEX,
    ERR_VM_POS_OOB,
    ERR_UNDEFINED_VAR,
    ERR_WRONG_VAR_INIT,

    // CLI errors
    ERR_NO_ARGS = 200,
    ERR_INVALID_ARG,
    ERR_NO_INPUT

} ErrorType;

// Internal
static void error_oom() {

    fprintf(stderr, "╭ %sERROR [%d]: Out of memory%s\n|\n", FG_RED_BOLD, ERR_OOM, RESET);
    fprintf(stderr, "╰ %sNote:%s Reduce memory usage or increase its capacity.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_open_str(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Unterminated string (line %d)%s\n|\n", FG_RED_BOLD, ERR_OPEN_STR, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Use a closing '\"' to end a string.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_expect_symbol(int line, const char *expected) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected %s (line %d) %s\n|\n", FG_RED_BOLD, ERR_EXPECT_SYMBOL, expected, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Missing symbol is required.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_expect_expression(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected expression (line %d)%s\n|\n", FG_RED_BOLD, ERR_EXPECT_EXPRESSION, line, RESET);
    fprintf(stderr, "╰ %sNote:%s An expression is required following a statement.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_expect_statement(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected statement or declaration (line %d)%s\n|\n", FG_RED_BOLD, ERR_EXPECT_STATEMENT, line, RESET);
    fprintf(stderr, "╰ %sNote:%s A statement or declaration is required.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_invalid_token(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Unexpected token at global scope (line %d)%s\n|\n", FG_RED_BOLD, ERR_INVALID_TOK, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Only 'entry' block is permitted in global scope.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_multiple_entry() {

    fprintf(stderr, "╭ %sERROR [%d]: Multiple program entrypoints found%s\n|\n", FG_RED_BOLD, ERR_MANY_ENTRY, RESET);
    fprintf(stderr, "╰ %sNote:%s Only 1 'entry' block is permitted.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_no_entry() {

    fprintf(stderr, "╭ %sERROR [%d]: No program entrypoint found%s\n|\n", FG_RED_BOLD, ERR_NO_ENTRY, RESET);
    fprintf(stderr, "╰ %sNote:%s An 'entry' block must be used.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_type_mismatch(const char *var_name, const char *expected_type, const char *actual_type) {

    fprintf(stderr, "╭ %sERROR [%d]: Type mismatch for variable '%s'%s\n|\n", FG_RED_BOLD, ERR_TYPE_MISMATCH, var_name, RESET);
    fprintf(stderr, "╰ %sNote:%s Expected %s but recieved %s.\n", FG_BLUE_BOLD, RESET, expected_type, actual_type);
    exit(1);

}

static void error_invalid_opcode(int op) {

    fprintf(stderr, "╭ %sERROR [%d]: Opcode '%d' is unknown%s\n|\n", FG_RED_BOLD, ERR_INVALID_OPCODE, op, RESET);
    fprintf(stderr, "╰ %sNote:%s Internal error.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_invalid_var_index(size_t var_count) {

    fprintf(stderr, "╭ %sERROR [%d]: Invalid variable index%s\n|\n", FG_RED_BOLD, ERR_INVALID_VAR_INDEX, RESET);
    fprintf(stderr, "╰ %sNote:%s Only up to %zu variables are allowed.\n", FG_BLUE_BOLD, RESET, var_count);
    exit(1);

}

static void error_invalid_const_index(size_t const_count) {

    fprintf(stderr, "╭ %sERROR [%d]: Invalid constant index%s\n|\n", FG_RED_BOLD, ERR_INVALID_CONST_INDEX, RESET);
    fprintf(stderr, "╰ %sNote:%s Only up to %zu constants are allowed.\n", FG_BLUE_BOLD, RESET, const_count);
    exit(1);

}

static void error_vm_oob() {

    fprintf(stderr, "╭ %sERROR [%d]: VM pointer out of bounds%s\n|\n", FG_RED_BOLD, ERR_VM_POS_OOB, RESET);
    fprintf(stderr, "╰ %sNote:%s Internal error.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_wrong_var_init(size_t var_count, size_t init_count) {

    fprintf(stderr, "╭ %sERROR [%d]: Variable initialization mismatch%s\n|\n", FG_RED_BOLD, ERR_WRONG_VAR_INIT, RESET);
    fprintf(stderr, "╰ %sNote:%s %zu variables declared but %zu initializers exist.\n", FG_BLUE_BOLD, RESET, var_count, init_count);
    exit(1);

}

static void error_undefined_var(const char *name) {

    fprintf(stderr, "╭ %sERROR [%d]: Variable '%s' is undefined%s\n|\n", FG_RED_BOLD, ERR_UNDEFINED_VAR, name, RESET);
    fprintf(stderr, "╰ %sNote:%s Variables must be declared before use.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

// CLI
static void error_no_args() {

    fprintf(stderr, "╭ %sERROR [%d]: Insufficient arguments%s\n|\n", FG_RED_BOLD, ERR_NO_ARGS, RESET);
    fprintf(stderr, "╰ %sNote:%s At least 1 argument required (<input_file.phase>).\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_invalid_arg(const char *arg) {

    fprintf(stderr, "╭ %sERROR [%d]: Argument '%s' is invalid%s\n|\n", FG_RED_BOLD, ERR_INVALID_ARG, arg, RESET);
    fprintf(stderr, "╰ %sNote:%s See all valid arguments with 'phase --help'.\n", FG_BLUE_BOLD, RESET);
    exit(1);

}

static void error_ifnf(const char *name) {

    fprintf(stderr, "╭ %sERROR [%d]: Input file '%s' not found%s\n|\n", FG_RED_BOLD, ERR_NO_INPUT, name, RESET);
    fprintf(stderr, "╰ %sNote:%s A valid input file path is required (e.g. path/to/file.phase).\n", FG_BLUE_BOLD, RESET);
    exit(1);

}
