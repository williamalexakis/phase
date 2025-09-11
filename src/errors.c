#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colours.h"

typedef enum {

    E_OOM = 1,
    E_UNTERMINATED_STR,
    E_EXPECT_GENERIC,
    E_EXPECT_EXPRESSION,
    E_EXPECT_STATEMENT,
    E_INVALID_TOK,

    E_NO_ARGS,
    E_INVALID_ARG,
    E_IFNF,

} ErrorType;

static void error_oom() {

    fprintf(stderr, "%sERROR [%d]: Out of memory%s\n\n", FG_RED_BOLD, E_OOM, RESET);
    fprintf(stderr, "%sNote:%s Reduce memory usage or increase its capacity.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_no_args() {

    fprintf(stderr, "%sERROR [%d]: Insufficient arguments%s\n\n", FG_RED_BOLD, E_NO_ARGS, RESET);
    fprintf(stderr, "%sNote:%s At least 1 argument required (<input_file.phase>).\n", FG_BLUE, RESET);
    exit(1);

}

static void error_invalid_arg(const char *arg) {

    fprintf(stderr, "%sERROR [%d]: Argument '%s' is invalid%s\n\n", FG_RED_BOLD, E_INVALID_ARG, arg, RESET);
    fprintf(stderr, "%sNote:%s See all valid arguments with 'phase --help'.\n", FG_BLUE, RESET);
    exit(1);

}

static void error_ifnf(const char *name) {

    fprintf(stderr, "%sERROR [%d]: Input file '%s' not found%s\n\n", FG_RED_BOLD, E_IFNF, name, RESET);
    fprintf(stderr, "%sNote:%s A valid input file path is required (e.g. path/to/file.phase).\n", FG_BLUE, RESET);
    exit(1);

}
