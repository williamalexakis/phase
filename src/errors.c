#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void error_no_args() {

    fprintf(stderr, "ERROR: Insufficient arguments provided\n\n");
    fprintf(stderr, "Note: At least 2 args are required.\n");
    exit(1);

}
