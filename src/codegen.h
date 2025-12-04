#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "parser.h"

typedef enum {

    OP_PUSH_CONST,
    OP_PRINT,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_CALL,
    OP_RET,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_POP,
    OP_HALT

} Opcode;

typedef enum {

    VAL_STRING,
    VAL_INTEGER,
    VAL_FLOAT,
    VAL_BOOLEAN,
    VAL_VOID

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

    char *name;
    TokenType return_type;
    TokenType *param_types;
    size_t param_count;
    bool has_return;
    char **local_names;
    TokenType *local_types;
    size_t local_count;
    size_t local_cap;
    size_t start_ip;

} FunctionDef;

typedef struct {

    uint8_t *code;
    size_t code_len;
    size_t code_cap;

    Value *constants;
    size_t const_count;
    size_t const_cap;

    char **global_names;
    TokenType *global_types;
    size_t global_count;
    size_t global_cap;

    FunctionDef entry;

    FunctionDef *functions;
    size_t func_count;
    size_t func_cap;

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

    Value *globals;
    size_t global_count;

    FunctionDef *functions;
    size_t func_count;
    FunctionDef entry_fn;

    struct CallFrame *frames;
    size_t frame_count;
    size_t frame_cap;

} VM;

void emit_program(Emitter *emitter, AstProgram *program);
void free_emitter(Emitter *emitter);
void init_vm(VM *vm, Value *constants, size_t const_count, uint8_t *code, size_t code_len, FunctionDef *functions, size_t func_count, FunctionDef entry_fn, size_t global_count);
void free_vm(VM *vm);
void interpret(VM *vm);
const char *token_type_to_string(TokenType type);

#endif
