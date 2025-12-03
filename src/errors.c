#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "colours.h"
#include "errors.h"

typedef struct {

    ErrorType code;
    const char *message_fmt;
    const char *help_fmt;
    const char *error_colour;
    const char *help_colour;

} ErrorInfo;

static const ErrorInfo ERROR_TABLE[] = {

    { ERR_OOM, "Out of memory.", "Reduce memory usage or increase its capacity.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_OPEN_STR, "Unterminated string (line %d).", "Use a closing '\"' to end a string.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_EXPECT_SYMBOL, "Expected %s (line %d).", "Implement the missing symbol.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_EXPECT_EXPRESSION, "Expected expression (line %d).", "Implement an expression after a statement.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_EXPECT_STATEMENT, "Expected statement or declaration (line %d).", "Implement a statement or declaration.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_INVALID_TOK, "Unexpected token at global scope (line %d).", "Remove the invalid symbol; only 'entry' blocks are valid in global scope.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_MANY_ENTRY, "Multiple program entrypoints found.", "Use only 1 'entry' block.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_NO_ENTRY, "No program entrypoint found.", "You must have an 'entry' block to signify program entrypoint.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_TYPE_MISMATCH, "Type mismatch for variable '%s'.", "Use %s instead of the current %s.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_INVALID_OPCODE, "Opcode '%d' is unknown.", "Unavailable (INTERNAL ERROR).", FG_RED_BOLD, FG_PURPLE_BOLD },
    { ERR_INVALID_VAR_INDEX, "Invalid variable index.", "Reduce total amount of variables; only up to %zu variables are allowed.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_INVALID_CONST_INDEX, "Invalid constant index.", "Reduce total amount of constants; only up to %zu constants are allowed.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_VM_POS_OOB, "VM pointer out of bounds.", "Unavailable (INTERNAL ERROR).", FG_RED_BOLD, FG_PURPLE_BOLD },
    { ERR_UNDEFINED_VAR, "Variable '%s' is undefined.", "Variables must be declared before use.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_WRONG_VAR_INIT, "Variable initialization mismatch.", "%zu variables declared but %zu initializers exist.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_NO_ARGS, "Insufficient arguments.", "At least 1 argument is required (<input_file.phase>).", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_INVALID_ARG, "Argument '%s' is invalid.", "See all available arguments with 'phase --help'.", FG_RED_BOLD, FG_BLUE_BOLD },
    { ERR_NO_INPUT, "Input file '%s' not found.", "Use a valid input path (e.g. /path/to/file.phase).", FG_RED_BOLD, FG_BLUE_BOLD }

};

static const ErrorInfo *find_error_info(ErrorType code) {

    size_t count = sizeof(ERROR_TABLE) / sizeof(ERROR_TABLE[0]);

    for (size_t i = 0; i < count; i++) {

        if (ERROR_TABLE[i].code == code) return &ERROR_TABLE[i];

    }

    return NULL;

}

static noreturn void error_emit(ErrorType code, ...) {

    const ErrorInfo *info = find_error_info(code);

    if (!info) {

        fprintf(stderr, "%s┃ ERROR [%d]:%s Unknown error.\n", FG_RED_BOLD, code, RESET);
        fprintf(stderr, "%s┃ Help:%s Unavailable (INTERNAL ERROR).\n", FG_PURPLE_BOLD, RESET);
        exit(1);

    }

    va_list args;
    va_start(args, code);

    va_list args_msg;
    va_copy(args_msg, args);

    fprintf(stderr, "%s┃ ERROR [%d]:%s ", info->error_colour, info->code, RESET);
    vfprintf(stderr, info->message_fmt, args_msg);
    fprintf(stderr, "\n");

    va_end(args_msg);

    va_list args_help;
    va_copy(args_help, args);

    fprintf(stderr, "%s┃ Help:%s ", info->help_colour, RESET);
    vfprintf(stderr, info->help_fmt, args_help);
    fprintf(stderr, "\n");

    va_end(args_help);
    va_end(args);

    exit(1);

}

// Internal errors
void error_oom(void) { error_emit(ERR_OOM); }
void error_open_str(int line) { error_emit(ERR_OPEN_STR, line); }
void error_expect_symbol(int line, const char *expected) { error_emit(ERR_EXPECT_SYMBOL, expected, line); }
void error_expect_expression(int line) { error_emit(ERR_EXPECT_EXPRESSION, line); }
void error_expect_statement(int line) { error_emit(ERR_EXPECT_STATEMENT, line); }
void error_invalid_token(int line) { error_emit(ERR_INVALID_TOK, line); }
void error_multiple_entry(void) { error_emit(ERR_MANY_ENTRY); }
void error_no_entry(void) { error_emit(ERR_NO_ENTRY); }
void error_type_mismatch(const char *var_name, const char *expected_type, const char *actual_type) {
    error_emit(ERR_TYPE_MISMATCH, var_name, expected_type, actual_type);
}

void error_invalid_opcode(int op) { error_emit(ERR_INVALID_OPCODE, op); }
void error_invalid_var_index(size_t var_count) { error_emit(ERR_INVALID_VAR_INDEX, var_count); }
void error_invalid_const_index(size_t const_count) { error_emit(ERR_INVALID_CONST_INDEX, const_count); }
void error_vm_oob(void) { error_emit(ERR_VM_POS_OOB); }
void error_wrong_var_init(size_t var_count, size_t init_count) { error_emit(ERR_WRONG_VAR_INIT, var_count, init_count); }
void error_undefined_var(const char *name) { error_emit(ERR_UNDEFINED_VAR, name); }

// CLI errors
void error_no_args(void) { error_emit(ERR_NO_ARGS); }
void error_invalid_arg(const char *arg) { error_emit(ERR_INVALID_ARG, arg); }
void error_ifnf(const char *name) { error_emit(ERR_NO_INPUT, name); }
