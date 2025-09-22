#include "parser.c"
#include <stdint.h>

typedef enum {

    OP_PUSH_CONST,
    OP_PRINT,
    OP_SET_VAR,
    OP_GET_VAR,
    OP_HALT

} Opcode;

typedef enum {

    VAL_STRING,
    VAL_INTEGER

} ValueType;

typedef struct {

    ValueType type;

    union {

        char *str;
        int integer;

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

static void init_emitter(Emitter *emitter) {

    emitter->code = NULL;
    emitter->code_len = 0;
    emitter->code_cap = 0;

    emitter->constants = NULL;
    emitter->const_count = 0;
    emitter->const_cap = 0;

    emitter->var_names = NULL;
    emitter->var_types = NULL;
    emitter->var_count = 0;
    emitter->var_cap = 0;

}

static void free_emitter(Emitter *emitter) {

    free(emitter->code);

    for (size_t i = 0; i < emitter->const_count; i++) {

        if (emitter->constants[i].type == VAL_STRING) {

            free(emitter->constants[i].as.str);

        }

    }

    free(emitter->constants);

    for (size_t i = 0; i < emitter->var_count; i++) {
        free(emitter->var_names[i]);
    }

    free(emitter->var_names);
    free(emitter->var_types);

}

static void emit_byte(Emitter *emitter, uint8_t byte) {

    if (emitter->code_len + 1 > emitter->code_cap) {

        size_t new_cap = emitter->code_cap ? emitter->code_cap * 2 : 64;
        emitter->code = realloc(emitter->code, new_cap);

        if (!emitter->code) error_oom();

        emitter->code_cap = new_cap;

    }

    emitter->code[emitter->code_len++] = byte;

}

static void emit_u16(Emitter *emitter, uint16_t value) {

    emit_byte(emitter, (value >> 8) & 0xFF);
    emit_byte(emitter, value & 0xFF);

}

static size_t add_constant(Emitter *emitter, Value value) {

    if (emitter->const_count + 1 > emitter->const_cap) {

        size_t new_cap = emitter->const_cap ? emitter->const_cap * 2 : 8;
        emitter->constants = realloc(emitter->constants, new_cap * sizeof(Value));

        if (!emitter->constants) error_oom();

        emitter->const_cap = new_cap;

    }

    emitter->constants[emitter->const_count] = value;

    return emitter->const_count++;

}

static size_t add_variable(Emitter *emitter, const char *name, TokenType type) {

    if (emitter->var_count + 1 > emitter->var_cap) {

        size_t new_cap = emitter->var_cap ? emitter->var_cap * 2 : 8;
        emitter->var_names = realloc(emitter->var_names, new_cap * sizeof(char*));
        emitter->var_types = realloc(emitter->var_types, new_cap * sizeof(TokenType));

        if (!emitter->var_names || !emitter->var_types) error_oom();

        emitter->var_cap = new_cap;

    }

    emitter->var_names[emitter->var_count] = strdup(name);
    emitter->var_types[emitter->var_count] = type;

    return emitter->var_count++;

}

static size_t find_variable(Emitter *emitter, const char *name) {

    for (size_t i = 0; i < emitter->var_count; i++) {

        if (strcmp(emitter->var_names[i], name) == 0) {

            return i;

        }

    }

    return SIZE_MAX;  // Not found

}

static TokenType get_variable_type(Emitter *emitter, size_t var_indx) {

    if (var_indx >= emitter->var_count) {
        return TOK_UNKNOWN;
    }

    return emitter->var_types[var_indx];

}

static TokenType get_expression_type(Emitter *emitter, AstExpression *expression) {

    switch (expression->tag) {
        case EXP_STRING: return TOK_STRING_T;
        case EXP_INTEGER: return TOK_INTEGER_T;
        case EXP_VARIABLE: {
            size_t var_indx = find_variable(emitter, expression->variable.name);
            if (var_indx == SIZE_MAX) {
                fprintf(stderr, "UNDEFINED VARIABLE: %s\n", expression->variable.name);
                exit(1);
            }
            return get_variable_type(emitter, var_indx);
        }
        default: return TOK_UNKNOWN;
    }

}

static const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOK_STRING_T: return "str";
        case TOK_INTEGER_T: return "int";
        default: return "unknown";
    }
}

static void emit_expression(Emitter *emitter, AstExpression *expression) {

    switch (expression->tag) {

        case EXP_STRING: {

            Value value = {

                .type = VAL_STRING,
                .as.str = strdup(expression->str_lit.value)

            };
            size_t indx = add_constant(emitter, value);
            emit_byte(emitter, OP_PUSH_CONST);
            emit_u16(emitter, (uint16_t)indx);

        } break;

        case EXP_INTEGER: {

            Value value = {

                .type = VAL_INTEGER,
                .as.integer = expression->int_lit.value

            };
            size_t indx = add_constant(emitter, value);
            emit_byte(emitter, OP_PUSH_CONST);
            emit_u16(emitter, (uint16_t)indx);

        } break;

        case EXP_VARIABLE: {

            size_t var_indx = find_variable(emitter, expression->variable.name);

            if (var_indx == SIZE_MAX) {

                fprintf(stderr, "UNDEFINED VARIABLE: %s\n", expression->variable.name);
                exit(1);

            }

            emit_byte(emitter, OP_GET_VAR);
            emit_u16(emitter, (uint16_t)var_indx);

        } break;

    }

}

static void emit_statement(Emitter *emitter, AstStatement *statement) {

    switch (statement->tag) {

        case STM_OUT: {

            emit_expression(emitter, statement->out.expression);
            emit_byte(emitter, OP_PRINT);

        } break;

        case STM_ASSIGN: {

            size_t var_indx = find_variable(emitter, statement->assign.var_name);

            if (var_indx == SIZE_MAX) {

                fprintf(stderr, "UNDEFINED VARIABLE: %s\n", statement->assign.var_name);
                exit(1);

            }

            TokenType var_type = get_variable_type(emitter, var_indx);
            TokenType expr_type = get_expression_type(emitter, statement->assign.expression);

            if (var_type != expr_type) {
                error_type_mismatch(statement->assign.var_name, 
                                   token_type_to_string(var_type), 
                                   token_type_to_string(expr_type));
            }

            emit_expression(emitter, statement->assign.expression);
            emit_byte(emitter, OP_SET_VAR);
            emit_u16(emitter, (uint16_t)var_indx);

        } break;

        case STM_VAR_DECL: {

            // Check if we have mismatched counts for initialization
            if (statement->var_decl.init_count > 0 && 
                statement->var_decl.init_count != statement->var_decl.var_count) {
                fprintf(stderr, "INITIALIZATION ERROR: %zu variables but %zu initializers\n", 
                        statement->var_decl.var_count, statement->var_decl.init_count);
                exit(1);
            }

            // Add all variables and handle initialization
            for (size_t i = 0; i < statement->var_decl.var_count; i++) {
                size_t var_indx = add_variable(emitter, statement->var_decl.var_names[i], statement->var_decl.var_type);

                if (i < statement->var_decl.init_count) {
                    TokenType var_type = statement->var_decl.var_type;
                    TokenType expr_type = get_expression_type(emitter, statement->var_decl.init_exprs[i]);

                    if (var_type != expr_type) {
                        error_type_mismatch(statement->var_decl.var_names[i], 
                                           token_type_to_string(var_type), 
                                           token_type_to_string(expr_type));
                    }

                    emit_expression(emitter, statement->var_decl.init_exprs[i]);
                    emit_byte(emitter, OP_SET_VAR);
                    emit_u16(emitter, (uint16_t)var_indx);
                }
            }

        } break;

    }

}

static void emit_block(Emitter *emitter, AstBlock *block) {

    for (size_t i = 0; i < block->len; i++) {

        emit_statement(emitter, block->statements[i]);

    }

}

static void emit_declaration(Emitter *emitter, AstDeclaration *declare, bool *entry_exists) {

    switch (declare->tag) {

        case DEC_ENTRY: {

            if (*entry_exists) {

                error_multiple_entry();

            }

            emit_block(emitter, declare->entry.block);

            *entry_exists = true;

        } break;

        case DEC_VAR: {

            for (size_t i = 0; i < declare->var_decl.var_count; i++) {
                add_variable(emitter, declare->var_decl.var_names[i], declare->var_decl.var_type);
            }

        } break;

    }

}

static void emit_program(Emitter *emitter, AstProgram *program) {

    init_emitter(emitter);

    bool entry_exists = false;

    for (size_t i = 0; i < program->len; i++) {

        emit_declaration(emitter, program->declarations[i], &entry_exists);

    }

    if (!entry_exists) error_no_entry();

    emit_byte(emitter, OP_HALT);

}

typedef struct {

    Value *stack;
    size_t stack_count;
    size_t stack_cap;

    Value *constants;
    size_t const_count;

    uint8_t *code;
    size_t code_len;

    size_t pos;  // Instruction pointer (but I just call it position)

    Value *variables;
    size_t var_count;

} VM;

static void init_vm(VM *vm, Value *constants, size_t const_count, uint8_t *code, size_t code_len) {

    vm->stack = NULL;
    vm->stack_count = 0;
    vm->stack_cap = 0;

    vm->constants = constants;
    vm->const_count = const_count;

    vm->code = code;
    vm->code_len = code_len;

    vm->pos = 0;

    vm->variables = calloc(256, sizeof(Value));  // Support up to 256 variables
    vm->var_count = 256;

}

static void free_vm(VM *vm) {

    free(vm->stack);
    free(vm->variables);

}

static void push(VM *vm, Value value) {

    if (vm->stack_count + 1 > vm->stack_cap) {

        size_t new_cap = vm->stack_cap ? vm->stack_cap * 2 : 8;
        vm->stack = realloc(vm->stack, new_cap * sizeof(Value));

        if (!vm->stack) error_oom();

        vm->stack_cap = new_cap;

    }

    vm->stack[vm->stack_count++] = value;

}

static Value pop(VM *vm) {

    return vm->stack[--vm->stack_count];

}

static uint8_t read_byte(VM *vm) {

    return vm->code[vm->pos++];

}

static uint16_t read_u16(VM *vm) {

    uint16_t high = read_byte(vm);
    uint16_t low = read_byte(vm);

    return (high << 8) | low;

}

/* Execution loop */
static void interpret(VM *vm) {

    for (;;) {

        if (vm->pos >= vm->code_len) {

            fprintf(stderr, "VM POS OUT OF BOUNDS\n");
            exit(1);

        }

        Opcode operation = (Opcode)read_byte(vm);

        switch (operation) {

            case OP_PUSH_CONST: {

                uint16_t indx = read_u16(vm);

                if (indx >= vm->const_count) {

                    fprintf(stderr, "INVALID CONSTANT INDEX\n");
                    exit(1);

                }

                push(vm, vm->constants[indx]);

            } break;

            case OP_PRINT: {

                Value value = pop(vm);

                if (value.type == VAL_STRING) {

                    printf("%s\n", value.as.str);

                } else if (value.type == VAL_INTEGER) {

                    printf("%d\n", value.as.integer);

                } else {

                    fprintf(stderr, "PRINT EXPECTS STRING OR INTEGER\n");
                    exit(1);

                }

            } break;

            case OP_SET_VAR: {

                uint16_t var_indx = read_u16(vm);

                if (var_indx >= vm->var_count) {

                    fprintf(stderr, "INVALID VARIABLE INDEX\n");
                    exit(1);

                }

                vm->variables[var_indx] = pop(vm);

            } break;

            case OP_GET_VAR: {

                uint16_t var_indx = read_u16(vm);

                if (var_indx >= vm->var_count) {

                    fprintf(stderr, "INVALID VARIABLE INDEX\n");
                    exit(1);

                }

                push(vm, vm->variables[var_indx]);

            } break;

            case OP_HALT: {

                return;

            } break;

            default: {

                fprintf(stderr, "UNKNOWN OPCODE %d\n", operation);
                exit(1);

            }

        }

    }

}
