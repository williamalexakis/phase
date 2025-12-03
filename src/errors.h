#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>
#include <stdnoreturn.h>

typedef struct {

    const char *file;
    int line;
    int col_start;
    int col_end;

} ErrorLocation;

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

noreturn void error_oom(void);
noreturn void error_open_str(ErrorLocation loc);
noreturn void error_expect_symbol(ErrorLocation loc, const char *expected);
noreturn void error_expect_expression(ErrorLocation loc);
noreturn void error_expect_statement(ErrorLocation loc);
noreturn void error_invalid_token(ErrorLocation loc);
noreturn void error_multiple_entry(ErrorLocation loc);
noreturn void error_no_entry(void);
noreturn void error_type_mismatch(ErrorLocation loc, const char *var_name, const char *expected_type, const char *actual_type);
noreturn void error_invalid_opcode(ErrorLocation loc, int op);
noreturn void error_invalid_var_index(ErrorLocation loc, size_t var_count);
noreturn void error_invalid_const_index(ErrorLocation loc, size_t const_count);
noreturn void error_vm_oob(ErrorLocation loc);
noreturn void error_wrong_var_init(ErrorLocation loc, size_t var_count, size_t init_count);
noreturn void error_undefined_var(ErrorLocation loc, const char *name);
noreturn void error_no_args(void);
noreturn void error_invalid_arg(const char *arg);
noreturn void error_ifnf(const char *name);
void error_set_source(const char *file);

#endif
