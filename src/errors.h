#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>

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

void error_oom(void);
void error_open_str(int line);
void error_expect_symbol(int line, const char *expected);
void error_expect_expression(int line);
void error_expect_statement(int line);
void error_invalid_token(int line);
void error_multiple_entry(void);
void error_no_entry(void);
void error_type_mismatch(const char *var_name, const char *expected_type, const char *actual_type);
void error_invalid_opcode(int op);
void error_invalid_var_index(size_t var_count);
void error_invalid_const_index(size_t const_count);
void error_vm_oob(void);
void error_wrong_var_init(size_t var_count, size_t init_count);
void error_undefined_var(const char *name);
void error_no_args(void);
void error_invalid_arg(const char *arg);
void error_ifnf(const char *name);

#endif
