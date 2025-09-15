#include "parser.c"
#include <stdint.h>

typedef enum {

    OP_PUSH_CONST,
    OP_PRINT,
    OP_HALT

} Opcode;

typedef enum {

    VAL_STRING

} ValueType;

typedef struct {

    ValueType type;

    union {

        char *str;

    } as;

} Value;

typedef struct {

    uint8_t *code;
    size_t code_len;
    size_t code_cap;

    Value *constants;
    size_t const_count;
    size_t const_cap;

} Emitter;

static void init_emitter(Emitter *emitter) {

    emitter->code = NULL;
    emitter->code_len = 0;
    emitter->code_cap = 0;

    emitter->constants = NULL;
    emitter->const_count = 0;
    emitter->const_cap = 0;

}

static void free_emitter(Emitter *emitter) {

    free(emitter->code);

    for (size_t i = 0; i < emitter->const_count; i++) {

        if (emitter->constants[i].type == VAL_STRING) {

            free(emitter->constants[i].as.str);

        }

    }

    free(emitter->constants);

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

static void emit_expression(Emitter *emitter, AstExpression *expression) {

    switch (expression->tag) {

        case E_STRING: {

            Value value = {

                .type = VAL_STRING,
                .as.str = strdup(expression->str_lit.value)

            };
            size_t indx = add_constant(emitter, value);
            emit_byte(emitter, OP_PUSH_CONST);
            emit_u16(emitter, (uint16_t)indx);

        } break;

    }

}

static void emit_statement(Emitter *emitter, AstStatement *statement) {

    switch (statement->tag) {

        case S_OUT: {

            emit_expression(emitter, statement->out.expression);
            emit_byte(emitter, OP_PRINT);

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

        case D_ENTRY: {

            if (*entry_exists) {

                error_multiple_entry();

            }

            emit_block(emitter, declare->entry.block);

            *entry_exists = true;

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

}

static void free_vm(VM *vm) {

    free(vm->stack);

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

                if (value.type != VAL_STRING) {

                    fprintf(stderr, "PRINT EXPECTS STRING\n");
                    exit(1);

                }

                printf("%s\n", value.as.str);  // Execute print

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
