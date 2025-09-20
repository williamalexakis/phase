#include <stdio.h>
#include <stdlib.h>
#include "colours.h"

typedef enum {

    ERR_OOM = 1,

    ERR_OPEN_STR,
    ERR_EXPECT_GENERIC,
    ERR_EXPECT_EXPRESSION,
    ERR_EXPECT_STATEMENT,
    ERR_INVALID_TOK,
    ERR_MANY_ENTRY,
    ERR_NO_ENTRY,
    ERR_TYPE_MISMATCH,

    ERR_NO_ARGS,
    ERR_INVALID_ARG,
    ERR_NO_INPUT

} ErrorType;

// General
static void error_oom() {

    fprintf(stderr, "╭ %sERROR [%d]: Out of memory%s\n|\n", FG_RED_BOLD, ERR_OOM, RESET);
    fprintf(stderr, "╰ %sNote:%s Reduce memory usage or increase its capacity.\n", FG_BLUE, RESET);
    exit(1);

}

// Source
static void error_open_str(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Unterminated string (line %d)%s\n|\n", FG_RED_BOLD, ERR_OPEN_STR, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Use a closing '\"' to end a string.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_expect_generic(int line, const char *expected) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected %s (line %d) %s\n|\n", FG_RED_BOLD, ERR_EXPECT_GENERIC, expected, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Missing symbol is required.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_expect_expression(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected expression (line %d)%s\n|\n", FG_RED_BOLD, ERR_EXPECT_EXPRESSION, line, RESET);
    fprintf(stderr, "╰ %sNote:%s An expression is required following a statement.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_expect_statement(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected statement (line %d)%s\n|\n", FG_RED_BOLD, ERR_EXPECT_STATEMENT, line, RESET);
    fprintf(stderr, "╰ %sNote:%s A statement or declaration is required.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_invalid_token(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Unexpected token at global scope (line %d)%s\n|\n", FG_RED_BOLD, ERR_INVALID_TOK, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Only 'entry' block is permitted in global scope.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_multiple_entry() {

    fprintf(stderr, "╭ %sERROR [%d]: Multiple program entrypoints found%s\n|\n", FG_RED_BOLD, ERR_MANY_ENTRY, RESET);
    fprintf(stderr, "╰ %sNote:%s Only 1 'entry' block is permitted.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_no_entry() {

    fprintf(stderr, "╭ %sERROR [%d]: No program entrypoint found%s\n|\n", FG_RED_BOLD, ERR_NO_ENTRY, RESET);
    fprintf(stderr, "╰ %sNote:%s An 'entry' block must be used.\n", FG_BLUE, RESET);
    exit(1);

}

// CLI
static void error_no_args() {

    fprintf(stderr, "╭ %sERROR [%d]: Insufficient arguments%s\n|\n", FG_RED_BOLD, ERR_NO_ARGS, RESET);
    fprintf(stderr, "╰ %sNote:%s At least 1 argument required (<input_file.phase>).\n", FG_BLUE, RESET);
    exit(1);

}

static void error_invalid_arg(const char *arg) {

    fprintf(stderr, "╭ %sERROR [%d]: Argument '%s' is invalid%s\n|\n", FG_RED_BOLD, ERR_INVALID_ARG, arg, RESET);
    fprintf(stderr, "╰ %sNote:%s See all valid arguments with 'phase --help'.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_ifnf(const char *name) {

    fprintf(stderr, "╭ %sERROR [%d]: Input file '%s' not found%s\n|\n", FG_RED_BOLD, ERR_NO_INPUT, name, RESET);
    fprintf(stderr, "╰ %sNote:%s A valid input file path is required (e.g. path/to/file.phase).\n", FG_BLUE, RESET);
    exit(1);

}

static void error_type_mismatch(const char *var_name, const char *expected_type, const char *actual_type) {

    fprintf(stderr, "╭ %sERROR [%d]: Type mismatch for variable '%s'%s\n|\n", FG_RED_BOLD, ERR_TYPE_MISMATCH, var_name, RESET);
    fprintf(stderr, "╰ %sNote:%s Expected %s, but got %s.\n", FG_BLUE, RESET, expected_type, actual_type);
    exit(1);

}
