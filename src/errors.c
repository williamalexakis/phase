#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colours.h"

typedef enum {

    E_OOM = 1,

    E_OPEN_STR,
    E_EXPECT_GENERIC,
    E_EXPECT_EXPRESSION,
    E_EXPECT_STATEMENT,
    E_INVALID_TOK,
    E_MANY_ENTRY,
    E_NO_ENTRY,

    E_NO_ARGS,
    E_INVALID_ARG,
    E_IFNF,

} ErrorType;

// General
static void error_oom() {

    fprintf(stderr, "╭ %sERROR [%d]: Out of memory%s\n|\n", FG_RED_BOLD, E_OOM, RESET);
    fprintf(stderr, "╰ %sNote:%s Reduce memory usage or increase its capacity.\n", FG_BLUE, RESET);
    exit(1);

}

// Source
static void error_open_str(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Unterminated string (line %d)%s\n|\n", FG_RED_BOLD, E_OPEN_STR, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Use a closing '\"' to end a string.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_expect_generic(int line, const char *expected) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected %s (line %d) %s\n|\n", FG_RED_BOLD, E_EXPECT_GENERIC, expected, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Missing symbol is required.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_expect_expression(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected expression (line %d)%s\n|\n", FG_RED_BOLD, E_EXPECT_EXPRESSION, line, RESET);
    fprintf(stderr, "╰ %sNote:%s An expression is required following a statement.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_expect_statement(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Expected statement (line %d)%s\n|\n", FG_RED_BOLD, E_EXPECT_STATEMENT, line, RESET);
    fprintf(stderr, "╰ %sNote:%s A statement or declaration is required.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_invalid_token(int line) {

    fprintf(stderr, "╭ %sERROR [%d]: Unexpected token at global scope (line %d)%s\n|\n", FG_RED_BOLD, E_INVALID_TOK, line, RESET);
    fprintf(stderr, "╰ %sNote:%s Only 'entry' block is permitted in global scope.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_multiple_entry() {

    fprintf(stderr, "╭ %sERROR [%d]: Multiple program entrypoints found%s\n|\n", FG_RED_BOLD, E_MANY_ENTRY, RESET);
    fprintf(stderr, "╰ %sNote:%s Only 1 'entry' block is permitted.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_no_entry() {

    fprintf(stderr, "╭ %sERROR [%d]: Multiple program entrypoints found%s\n|\n", FG_RED_BOLD, E_NO_ENTRY, RESET);
    fprintf(stderr, "╰ %sNote:%s An 'entry' block must be used.\n", FG_BLUE, RESET);
    exit(1);

}

// CLI
static void error_no_args() {

    fprintf(stderr, "╭ %sERROR [%d]: Insufficient arguments%s\n|\n", FG_RED_BOLD, E_NO_ARGS, RESET);
    fprintf(stderr, "╰ %sNote:%s At least 1 argument required (<input_file.phase>).\n", FG_BLUE, RESET);
    exit(1);

}

static void error_invalid_arg(const char *arg) {

    fprintf(stderr, "╭ %sERROR [%d]: Argument '%s' is invalid%s\n|\n", FG_RED_BOLD, E_INVALID_ARG, arg, RESET);
    fprintf(stderr, "╰ %sNote:%s See all valid arguments with 'phase --help'.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_ifnf(const char *name) {

    fprintf(stderr, "╭ %sERROR [%d]: Input file '%s' not found%s\n|\n", FG_RED_BOLD, E_IFNF, name, RESET);
    fprintf(stderr, "╰ %sNote:%s A valid input file path is required (e.g. path/to/file.phase).\n", FG_BLUE, RESET);
    exit(1);

}
