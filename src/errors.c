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
    E_UNRECOGNIZED_TOK,

    E_NO_ARGS,
    E_IFNF,
    E_OFNF

} ErrorType;

static void error_no_output() {

    fprintf(stderr, "%s%sERROR [%d]: Output file not found%s\n\n", FG_WHITE, BG_RED, E_OFNF, RESET);
    fprintf(stderr, "%sNote:%s A path to an output file is required unless an output flag is used.\n", FG_BLUE, RESET);
    exit(1);

}
