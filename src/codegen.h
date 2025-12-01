#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "parser.h"

typedef enum {

    OP_PUSH_CONST,
    OP_PRINT,
    OP_SET_VAR,
    OP_GET_VAR,
    OP_HALT

} Opcode;

typedef enum {

    VAL_STRING,
    VAL_INTEGER,
    VAL_FLOAT,
    VAL_BOOLEAN

} ValueType;

typedef struct {

    ValueType type;

    union {

        char *str;
        int integer;
        float floating;
        bool boolean;

    } as;

} Value;

typedef struct {

    uint8_t *code;
    size_t code_len;
    size_t code_cap;

    Value *constants;
    size_t const_count;
    size_t const_cap;

    char **var_names;
    TokenType *var_types;
    size_t var_count;
    size_t var_cap;

} Emitter;

typedef struct {

    Value *stack;
    size_t stack_count;
    size_t stack_cap;

    Value *constants;
    size_t const_count;

    uint8_t *code;
    size_t code_len;

    size_t pos;

    Value *variables;
    size_t var_count;

} VM;

void emit_program(Emitter *emitter, AstProgram *program);
void free_emitter(Emitter *emitter);
void init_vm(VM *vm, Value *constants, size_t const_count, uint8_t *code, size_t code_len);
void free_vm(VM *vm);
void interpret(VM *vm);
const char *token_type_to_string(TokenType type);

#endif
