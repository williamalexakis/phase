#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

typedef struct {

    char **names;
    TokenType *types;
    size_t count;
    size_t cap;

} VarTable;

typedef struct CallFrame {

    FunctionDef *fn;
    Value *locals;
    size_t return_ip;

} CallFrame;

static void zero_locals(Value *locals, size_t count) {

    for (size_t i = 0; i < count; i++) locals[i].type = VAL_VOID;

}
static size_t add_to_var_table(VarTable *table, const char *name, TokenType type) {

    if (table->count + 1 > table->cap) {

        size_t new_cap = table->cap ? table->cap * 2 : 8;
        table->names = realloc(table->names, new_cap * sizeof(char*));
        table->types = realloc(table->types, new_cap * sizeof(TokenType));

        if (!table->names || !table->types) error_oom();

        table->cap = new_cap;

    }

    table->names[table->count] = strdup(name);
    table->types[table->count] = type;

    return table->count++;

}

static size_t find_in_table(VarTable *table, const char *name) {

    for (size_t i = 0; i < table->count; i++) {

        if (strcmp(table->names[i], name) == 0) return i;

    }

    return SIZE_MAX;

}

static void init_emitter(Emitter *emitter) {

    emitter->code = NULL;
    emitter->code_len = 0;
    emitter->code_cap = 0;

    emitter->constants = NULL;
    emitter->const_count = 0;
    emitter->const_cap = 0;

    emitter->global_names = NULL;
    emitter->global_types = NULL;
    emitter->global_count = 0;
    emitter->global_cap = 0;

    emitter->functions = NULL;
    emitter->func_count = 0;
    emitter->func_cap = 0;

    emitter->entry.name = strdup("entry");
    emitter->entry.return_type = TOK_VOID_T;
    emitter->entry.param_types = NULL;
    emitter->entry.param_count = 0;
    emitter->entry.has_return = false;
    emitter->entry.local_names = NULL;
    emitter->entry.local_types = NULL;
    emitter->entry.local_count = 0;
    emitter->entry.local_cap = 0;
    emitter->entry.start_ip = 0;

}

static void free_function(FunctionDef *fn) {

    if (!fn) return;

    free(fn->name);
    free(fn->param_types);
    for (size_t i = 0; i < fn->local_count; i++) free(fn->local_names[i]);
    free(fn->local_names);
    free(fn->local_types);

}

void free_emitter(Emitter *emitter) {

    free(emitter->code);

    for (size_t i = 0; i < emitter->const_count; i++) {

        if (emitter->constants[i].type == VAL_STRING) free(emitter->constants[i].as.str);

    }

    free(emitter->constants);

    for (size_t i = 0; i < emitter->global_count; i++) free(emitter->global_names[i]);

    free(emitter->global_names);
    free(emitter->global_types);

    free_function(&emitter->entry);

    for (size_t i = 0; i < emitter->func_count; i++) free_function(&emitter->functions[i]);

    free(emitter->functions);

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

static size_t emit_jump(Emitter *emitter, Opcode op) {

    emit_byte(emitter, op);
    size_t jump_pos = emitter->code_len;
    emit_u16(emitter, 0);  // Lil placeholder
    return jump_pos;

}

static void patch_jump(Emitter *emitter, size_t jump_pos) {

    size_t target = emitter->code_len;
    emitter->code[jump_pos] = (target >> 8) & 0xFF;
    emitter->code[jump_pos + 1] = target & 0xFF;

}

static void emit_block(Emitter *emitter, FunctionDef *current_fn, AstBlock *block);

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

static size_t add_global(Emitter *emitter, const char *name, TokenType type) {

    if (emitter->global_count + 1 > emitter->global_cap) {

        size_t new_cap = emitter->global_cap ? emitter->global_cap * 2 : 8;
        emitter->global_names = realloc(emitter->global_names, new_cap * sizeof(char*));
        emitter->global_types = realloc(emitter->global_types, new_cap * sizeof(TokenType));

        if (!emitter->global_names || !emitter->global_types) error_oom();

        emitter->global_cap = new_cap;

    }

    emitter->global_names[emitter->global_count] = strdup(name);
    emitter->global_types[emitter->global_count] = type;

    return emitter->global_count++;

}

static size_t find_global(Emitter *emitter, const char *name) {

    for (size_t i = 0; i < emitter->global_count; i++) {

        if (strcmp(emitter->global_names[i], name) == 0) return i;

    }

    return SIZE_MAX;

}

static FunctionDef *find_function(Emitter *emitter, const char *name) {

    for (size_t i = 0; i < emitter->func_count; i++) {

        if (strcmp(emitter->functions[i].name, name) == 0) return &emitter->functions[i];

    }

    return NULL;

}

static FunctionDef *register_function(Emitter *emitter, const char *name, TokenType return_type, AstParam *params, size_t param_count) {

    if (find_function(emitter, name)) {

        ErrorLocation loc = {0};
        error_invalid_token(loc);

    }

    if (emitter->func_count + 1 > emitter->func_cap) {

        size_t new_cap = emitter->func_cap ? emitter->func_cap * 2 : 4;
        emitter->functions = realloc(emitter->functions, new_cap * sizeof(FunctionDef));

        if (!emitter->functions) error_oom();

        emitter->func_cap = new_cap;

    }

    FunctionDef *fn = &emitter->functions[emitter->func_count++];

    fn->name = strdup(name);
    fn->return_type = return_type;
    fn->param_count = param_count;
    fn->param_types = calloc(param_count, sizeof(TokenType));
    fn->has_return = false;
    fn->local_names = NULL;
    fn->local_types = NULL;
    fn->local_count = 0;
    fn->local_cap = 0;
    fn->start_ip = 0;

    if (param_count && !fn->param_types) error_oom();

    for (size_t i = 0; i < param_count; i++) fn->param_types[i] = params[i].type;

    return fn;

}

static size_t add_local(FunctionDef *fn, const char *name, TokenType type) {

    VarTable table = { .names = fn->local_names, .types = fn->local_types, .count = fn->local_count, .cap = fn->local_cap };
    size_t idx = add_to_var_table(&table, name, type);
    fn->local_names = table.names;
    fn->local_types = table.types;
    fn->local_count = table.count;
    fn->local_cap = table.cap;
    return idx;

}

static size_t find_local(FunctionDef *fn, const char *name) {

    VarTable table = { .names = fn->local_names, .types = fn->local_types, .count = fn->local_count };
    return find_in_table(&table, name);

}

static TokenType get_variable_type(Emitter *emitter, FunctionDef *current_fn, const char *name, bool *is_local_out, size_t *index_out) {

    size_t local_idx = find_local(current_fn, name);

    if (local_idx != SIZE_MAX) {

        if (is_local_out) *is_local_out = true;
        if (index_out) *index_out = local_idx;

        return current_fn->local_types[local_idx];

    }

    size_t global_idx = find_global(emitter, name);

    if (global_idx != SIZE_MAX) {

        if (is_local_out) *is_local_out = false;
        if (index_out) *index_out = global_idx;

        return emitter->global_types[global_idx];

    }

    return TOK_UNKNOWN;

}

static TokenType get_expression_type(Emitter *emitter, FunctionDef *current_fn, AstExpression *expression) {

    switch (expression->tag) {

        case EXP_STRING: return TOK_STRING_T;
        case EXP_INTEGER: return TOK_INTEGER_T;
        case EXP_FLOAT: return TOK_FLOAT_T;
        case EXP_BOOLEAN: return TOK_BOOLEAN_T;
        case EXP_VARIABLE: {

            bool is_local = false;
            size_t idx = 0;
            TokenType t = get_variable_type(emitter, current_fn, expression->variable.name, &is_local, &idx);

            if (t == TOK_UNKNOWN) {

                ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                error_undefined_var(loc, expression->variable.name);

            }

            return t;

        }
        case EXP_CALL: {

            FunctionDef *fn = find_function(emitter, expression->call.func_name);

            if (!fn) {

                ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                error_undefined_func(loc, expression->call.func_name);

            }

            return fn->return_type;

        }

        case EXP_UNARY: {

            TokenType inner = get_expression_type(emitter, current_fn, expression->unary.expr);

            if (expression->unary.op == TOK_BANG || expression->unary.op == TOK_NOT) {

                if (inner != TOK_BOOLEAN_T) {

                    ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                    error_type_mismatch(loc, "not", "bool", token_type_to_string(inner));

                }

                return TOK_BOOLEAN_T;

            } else if (expression->unary.op == TOK_SUBTRACT) {

                if (inner != TOK_INTEGER_T && inner != TOK_FLOAT_T) {

                    ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                    error_type_mismatch(loc, "negation", "number", token_type_to_string(inner));

                }

                return inner;

            }

            return TOK_UNKNOWN;

        }

        case EXP_BINARY: {

            TokenType left_type = get_expression_type(emitter, current_fn, expression->binary.left);
            TokenType right_type = get_expression_type(emitter, current_fn, expression->binary.right);

            if (left_type != right_type) {

                ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                error_type_mismatch(loc, "binary op", token_type_to_string(left_type), token_type_to_string(right_type));

            }

            // Logic
            if (expression->binary.op == TOK_AND || expression->binary.op == TOK_OR) {

                if (left_type != TOK_BOOLEAN_T) {

                    ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                    error_type_mismatch(loc, "logical op", "bool", token_type_to_string(left_type));

                }

                return TOK_BOOLEAN_T;

            }

            // Equality
            if (expression->binary.op == TOK_EQUAL_EQUAL) {

                return TOK_BOOLEAN_T;

            }

            // Comparison
            if (expression->binary.op == TOK_LESS || expression->binary.op == TOK_GREATER || expression->binary.op == TOK_LESS_EQUAL || expression->binary.op == TOK_GREATER_EQUAL) {

                if (left_type != TOK_INTEGER_T && left_type != TOK_FLOAT_T) {

                    ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                    error_type_mismatch(loc, "comparison", "number", token_type_to_string(left_type));

                }

                return TOK_BOOLEAN_T;

            }

            if (left_type != TOK_INTEGER_T && left_type != TOK_FLOAT_T) {

                ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                error_type_mismatch(loc, "binary op", "number", token_type_to_string(left_type));

            }

            return left_type;

        }

        default: return TOK_UNKNOWN;

    }

}

const char *token_type_to_string(TokenType type) {

    switch (type) {

        case TOK_STRING_T: return "str";
        case TOK_INTEGER_T: return "int";
        case TOK_FLOAT_T: return "float";
        case TOK_BOOLEAN_T: return "bool";
        case TOK_VOID_T: return "void";
        default: return "unknown";

    }

}

static void emit_expression(Emitter *emitter, FunctionDef *current_fn, AstExpression *expression);

static void emit_statement(Emitter *emitter, FunctionDef *current_fn, AstStatement *statement) {

    switch (statement->tag) {

        case STM_OUT: {

            emit_expression(emitter, current_fn, statement->out.expression);
            emit_byte(emitter, OP_PRINT);

        } break;

        case STM_ASSIGN: {

            bool is_local = false;
            size_t var_indx = 0;
            TokenType var_type = get_variable_type(emitter, current_fn, statement->assign.var_name, &is_local, &var_indx);

            if (var_type == TOK_UNKNOWN) {

                ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                error_undefined_var(loc, statement->assign.var_name);

            }

            TokenType expr_type = get_expression_type(emitter, current_fn, statement->assign.expression);

            if (var_type != expr_type) {

                ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                error_type_mismatch(loc, statement->assign.var_name, token_type_to_string(var_type), token_type_to_string(expr_type));

            }

            emit_expression(emitter, current_fn, statement->assign.expression);

            if (is_local) {

                emit_byte(emitter, OP_SET_LOCAL);

            } else {

                emit_byte(emitter, OP_SET_GLOBAL);

            }

            emit_u16(emitter, (uint16_t)var_indx);

        } break;

        case STM_VAR_DECL: {

            if (statement->var_decl.init_count > 0 && statement->var_decl.init_count != statement->var_decl.var_count) {

                ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                error_wrong_var_init(loc, statement->var_decl.var_count, statement->var_decl.init_count);

            }

            for (size_t i = 0; i < statement->var_decl.var_count; i++) {

                size_t var_indx = add_local(current_fn, statement->var_decl.var_names[i], statement->var_decl.var_type);

                if (i < statement->var_decl.init_count) {

                    TokenType var_type = statement->var_decl.var_type;
                    TokenType expr_type = get_expression_type(emitter, current_fn, statement->var_decl.init_exprs[i]);

                    if (var_type != expr_type) {

                        ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                        error_type_mismatch(loc,
                            statement->var_decl.var_names[i],
                            token_type_to_string(var_type),
                            token_type_to_string(expr_type));

                    }

                    emit_expression(emitter, current_fn, statement->var_decl.init_exprs[i]);
                    emit_byte(emitter, OP_SET_LOCAL);
                    emit_u16(emitter, (uint16_t)var_indx);
                }
            }

        } break;

        case STM_RETURN: {

            if (current_fn->return_type == TOK_VOID_T) {

                if (statement->ret.expression) {

                    ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                    error_type_mismatch(loc, "return", "void", "non-void");

                }

                emit_byte(emitter, OP_RET);
                current_fn->has_return = true;
                break;

            }

            if (!statement->ret.expression) {

                ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                error_expect_symbol(loc, "return value");

            }

            TokenType expr_type = get_expression_type(emitter, current_fn, statement->ret.expression);

            if (expr_type != current_fn->return_type) {

                ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                error_type_mismatch(loc, "return", token_type_to_string(current_fn->return_type), token_type_to_string(expr_type));

            }

            emit_expression(emitter, current_fn, statement->ret.expression);
            emit_byte(emitter, OP_RET);
            current_fn->has_return = true;

        } break;

        case STM_EXPR: {

            TokenType expr_type = get_expression_type(emitter, current_fn, statement->expr.expression);
            emit_expression(emitter, current_fn, statement->expr.expression);

            if (expr_type != TOK_VOID_T) {

                emit_byte(emitter, OP_POP);

            }

        } break;

        case STM_IF: {

            TokenType cond_type = get_expression_type(emitter, current_fn, statement->if_stmt.condition);

            if (cond_type != TOK_BOOLEAN_T) {

                ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                error_type_mismatch(loc, "condition", "bool", token_type_to_string(cond_type));

            }

            emit_expression(emitter, current_fn, statement->if_stmt.condition);
            size_t jump_false = emit_jump(emitter, OP_JUMP_IF_FALSE);

            emit_block(emitter, current_fn, statement->if_stmt.then_block);

            if (statement->if_stmt.else_block) {

                size_t jump_end = emit_jump(emitter, OP_JUMP);
                patch_jump(emitter, jump_false);
                emit_block(emitter, current_fn, statement->if_stmt.else_block);
                patch_jump(emitter, jump_end);

            } else {

                patch_jump(emitter, jump_false);

            }

        } break;

        case STM_WHILE: {

            size_t loop_start = emitter->code_len;

            TokenType cond_type = get_expression_type(emitter, current_fn, statement->if_stmt.condition);

            if (cond_type != TOK_BOOLEAN_T) {

                ErrorLocation loc = { .line = statement->line, .col_start = statement->column_start, .col_end = statement->column_end };
                error_type_mismatch(loc, "condition", "bool", token_type_to_string(cond_type));

            }

            emit_expression(emitter, current_fn, statement->if_stmt.condition);
            size_t exit_jump = emit_jump(emitter, OP_JUMP_IF_FALSE);

            emit_block(emitter, current_fn, statement->if_stmt.then_block);

            emit_byte(emitter, OP_JUMP);
            emit_u16(emitter, (uint16_t)loop_start);

            patch_jump(emitter, exit_jump);

        } break;

    }

}

static void emit_expression(Emitter *emitter, FunctionDef *current_fn, AstExpression *expression) {

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

        case EXP_FLOAT: {

            Value value = {

                .type = VAL_FLOAT,
                .as.floating = expression->float_lit.value

            };

            size_t indx = add_constant(emitter, value);

            emit_byte(emitter, OP_PUSH_CONST);
            emit_u16(emitter, (uint16_t)indx);

        } break;

        case EXP_BOOLEAN: {

            Value value = {

                .type = VAL_BOOLEAN,
                .as.boolean = expression->bool_lit.value

            };

            size_t indx = add_constant(emitter, value);

            emit_byte(emitter, OP_PUSH_CONST);
            emit_u16(emitter, (uint16_t)indx);

        } break;

        case EXP_VARIABLE: {

            bool is_local = false;
            size_t var_indx = 0;
            TokenType t = get_variable_type(emitter, current_fn, expression->variable.name, &is_local, &var_indx);

            if (t == TOK_UNKNOWN) {

                ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                error_undefined_var(loc, expression->variable.name);

            }

            emit_byte(emitter, is_local ? OP_GET_LOCAL : OP_GET_GLOBAL);
            emit_u16(emitter, (uint16_t)var_indx);

        } break;

        case EXP_CALL: {

            FunctionDef *fn = find_function(emitter, expression->call.func_name);

            if (!fn) {

                ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                error_unexpected_ident(loc, expression->call.func_name);

            }

            if (expression->call.arg_count != fn->param_count) {

                ErrorLocation loc = { .line = expression->line, .col_start = expression->column_start, .col_end = expression->column_end };
                error_wrong_var_init(loc, fn->param_count, expression->call.arg_count);

            }

            for (size_t i = 0; i < expression->call.arg_count; i++) {

                TokenType arg_type = get_expression_type(emitter, current_fn, expression->call.args[i]);
                TokenType param_type = fn->param_types[i];

                if (arg_type != param_type) {

                    ErrorLocation loc = { .line = expression->call.args[i]->line, .col_start = expression->call.args[i]->column_start, .col_end = expression->call.args[i]->column_end };
                    error_type_mismatch(loc, fn->name, token_type_to_string(param_type), token_type_to_string(arg_type));

                }

                emit_expression(emitter, current_fn, expression->call.args[i]);

            }

            size_t fn_index = (size_t)(fn - emitter->functions);

            emit_byte(emitter, OP_CALL);
            emit_u16(emitter, (uint16_t)fn_index);

        } break;

        case EXP_UNARY: {

            emit_expression(emitter, current_fn, expression->unary.expr);

            if (expression->unary.op == TOK_BANG || expression->unary.op == TOK_NOT) {

                emit_byte(emitter, OP_NOT);

            } else if (expression->unary.op == TOK_SUBTRACT) {

                emit_byte(emitter, OP_NEG);

            } else {

                error_invalid_opcode((ErrorLocation){0}, expression->unary.op);

            }

        } break;

        case EXP_BINARY: {

            emit_expression(emitter, current_fn, expression->binary.left);
            emit_expression(emitter, current_fn, expression->binary.right);

            switch (expression->binary.op) {

                case TOK_ADD: emit_byte(emitter, OP_ADD); break;
                case TOK_SUBTRACT: emit_byte(emitter, OP_SUB); break;
                case TOK_MULTIPLY: emit_byte(emitter, OP_MUL); break;
                case TOK_DIVIDE: emit_byte(emitter, OP_DIV); break;
                case TOK_AND: emit_byte(emitter, OP_AND); break;
                case TOK_OR: emit_byte(emitter, OP_OR); break;
                case TOK_EQUAL_EQUAL: emit_byte(emitter, OP_EQUAL); break;
                case TOK_LESS: emit_byte(emitter, OP_LESS); break;
                case TOK_GREATER: emit_byte(emitter, OP_GREATER); break;
                case TOK_LESS_EQUAL: emit_byte(emitter, OP_LESS_EQUAL); break;
                case TOK_GREATER_EQUAL: emit_byte(emitter, OP_GREATER_EQUAL); break;
                default: error_invalid_opcode((ErrorLocation){0}, expression->binary.op);

            }

        } break;

    }

}

static void emit_block(Emitter *emitter, FunctionDef *current_fn, AstBlock *block) {

    for (size_t i = 0; i < block->len; i++) emit_statement(emitter, current_fn, block->statements[i]);

}

static void emit_function(Emitter *emitter, FunctionDef *fn, AstDeclaration *declare) {

    fn->start_ip = emitter->code_len;
    fn->has_return = false;

    // The parameters become locals first
    for (size_t i = 0; i < declare->func.param_count; i++) add_local(fn, declare->func.params[i].name, declare->func.params[i].type);

    emit_block(emitter, fn, declare->func.body);

    if (fn->return_type == TOK_VOID_T && !fn->has_return) emit_byte(emitter, OP_RET);

    if (fn->return_type != TOK_VOID_T && !fn->has_return) {

        ErrorLocation loc = { .line = declare->line, .col_start = declare->column_start, .col_end = declare->column_end };
        error_missing_return(loc, fn->name);

    }

}

static void emit_declaration(Emitter *emitter, AstDeclaration *declare, bool *entry_exists) {

    switch (declare->tag) {

        case DEC_ENTRY: {

            if (*entry_exists) {

                ErrorLocation loc = { .line = declare->line, .col_start = declare->column_start, .col_end = declare->column_end };
                error_multiple_entry(loc);

            }

            emitter->entry.start_ip = emitter->code_len;
            emitter->entry.has_return = false;
            emit_block(emitter, &emitter->entry, declare->entry.block);
            emit_byte(emitter, OP_HALT);
            *entry_exists = true;

        } break;

        case DEC_VAR: {

            // Global vars are already registered in the first pass, so we do nothing

        } break;

        case DEC_FUNC: {

            FunctionDef *fn = find_function(emitter, declare->func.name);

            if (!fn) {

                ErrorLocation loc = { .line = declare->line, .col_start = declare->column_start, .col_end = declare->column_end };
                error_invalid_token(loc);

            }

            emit_function(emitter, fn, declare);

        } break;

    }

}

void emit_program(Emitter *emitter, AstProgram *program) {

    init_emitter(emitter);

    // First pass where we register functions and global vars
    for (size_t i = 0; i < program->len; i++) {

        AstDeclaration *decl = program->declarations[i];

        if (decl->tag == DEC_FUNC) {

            register_function(emitter, decl->func.name, decl->func.return_type, decl->func.params, decl->func.param_count);

        } else if (decl->tag == DEC_VAR) {

            for (size_t v = 0; v < decl->var_decl.var_count; v++) add_global(emitter, decl->var_decl.var_names[v], decl->var_decl.var_type);

        }

    }

    bool entry_exists = false;

    // Emit entry first so that it starts at IP 0
    for (size_t i = 0; i < program->len; i++) {

        if (program->declarations[i]->tag == DEC_ENTRY) {

            emit_declaration(emitter, program->declarations[i], &entry_exists);

        }

    }

    // We emit functions and globals
    for (size_t i = 0; i < program->len; i++) {

        if (program->declarations[i]->tag == DEC_FUNC || program->declarations[i]->tag == DEC_VAR) {

            emit_declaration(emitter, program->declarations[i], &entry_exists);

        }

    }

    if (!entry_exists) error_no_entry();

}

void init_vm(VM *vm, Value *constants, size_t const_count, uint8_t *code, size_t code_len, FunctionDef *functions, size_t func_count, FunctionDef entry_fn, size_t global_count) {

    vm->stack = NULL;
    vm->stack_count = 0;
    vm->stack_cap = 0;

    vm->constants = constants;
    vm->const_count = const_count;

    vm->code = code;
    vm->code_len = code_len;

    vm->pos = 0;

    vm->globals = calloc(global_count, sizeof(Value));
    vm->global_count = global_count;

    vm->functions = functions;
    vm->func_count = func_count;
    vm->entry_fn = entry_fn;

    vm->frames = NULL;
    vm->frame_count = 0;
    vm->frame_cap = 0;

    // Seed the entry frame locals
    CallFrame entry_frame = {
        .fn = &vm->entry_fn,
        .locals = calloc(entry_fn.local_count, sizeof(Value)),
        .return_ip = vm->code_len
    };

    if (entry_fn.local_count && !entry_frame.locals) error_oom();
    zero_locals(entry_frame.locals, entry_fn.local_count);

    vm->frame_cap = 4;
    vm->frames = malloc(vm->frame_cap * sizeof(CallFrame));
    if (!vm->frames) error_oom();

    vm->frames[vm->frame_count++] = entry_frame;

}

void free_vm(VM *vm) {

    free(vm->stack);

    for (size_t i = 0; i < vm->frame_count; i++) free(vm->frames[i].locals);

    free(vm->frames);
    free(vm->globals);

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

static CallFrame *current_frame(VM *vm) {

    if (vm->frame_count == 0) return NULL;
    return &vm->frames[vm->frame_count - 1];

}

void interpret(VM *vm) {

    for (;;) {

        if (vm->pos >= vm->code_len) error_vm_oob((ErrorLocation){0});

        Opcode operation = (Opcode)read_byte(vm);

        switch (operation) {

            case OP_PUSH_CONST: {

                uint16_t indx = read_u16(vm);

                if (indx >= vm->const_count) error_invalid_const_index((ErrorLocation){0}, vm->const_count);

                push(vm, vm->constants[indx]);

            } break;

            case OP_PRINT: {

                Value value = pop(vm);

                if (value.type == VAL_STRING) {

                    printf("%s\n", value.as.str);

                } else if (value.type == VAL_INTEGER) {

                    printf("%d\n", value.as.integer);

                } else if (value.type == VAL_FLOAT) {

                    printf("%g\n", value.as.floating);

                } else if (value.type == VAL_BOOLEAN) {

                    value.as.boolean ? printf("true\n") : printf("false\n");

                } else {

                    printf("void\n");

                }

            } break;

            case OP_SET_GLOBAL: {

                uint16_t var_indx = read_u16(vm);

                if (var_indx >= vm->global_count) error_invalid_var_index((ErrorLocation){0}, vm->global_count);

                vm->globals[var_indx] = pop(vm);

            } break;

            case OP_GET_GLOBAL: {

                uint16_t var_indx = read_u16(vm);

                if (var_indx >= vm->global_count) error_invalid_var_index((ErrorLocation){0}, vm->global_count);

                push(vm, vm->globals[var_indx]);

            } break;

            case OP_SET_LOCAL: {

                uint16_t var_indx = read_u16(vm);
                CallFrame *frame = current_frame(vm);

                if (!frame || var_indx >= frame->fn->local_count) error_invalid_var_index((ErrorLocation){0}, frame ? frame->fn->local_count : 0);

                frame->locals[var_indx] = pop(vm);

            } break;

            case OP_GET_LOCAL: {

                uint16_t var_indx = read_u16(vm);
                CallFrame *frame = current_frame(vm);

                if (!frame || var_indx >= frame->fn->local_count) error_invalid_var_index((ErrorLocation){0}, frame ? frame->fn->local_count : 0);

                push(vm, frame->locals[var_indx]);

            } break;

            case OP_CALL: {

                uint16_t fn_indx = read_u16(vm);

                if (fn_indx >= vm->func_count) error_invalid_opcode((ErrorLocation){0}, fn_indx);

                FunctionDef *fn = &vm->functions[fn_indx];

                Value *locals = calloc(fn->local_count, sizeof(Value));

                if (fn->local_count && !locals) error_oom();
                zero_locals(locals, fn->local_count);

                for (size_t i = 0; i < fn->param_count; i++) {

                    locals[fn->param_count - 1 - i] = pop(vm);

                }

                if (vm->frame_count + 1 > vm->frame_cap) {

                    size_t new_cap = vm->frame_cap ? vm->frame_cap * 2 : 4;
                    vm->frames = realloc(vm->frames, new_cap * sizeof(CallFrame));

                    if (!vm->frames) error_oom();

                    vm->frame_cap = new_cap;

                }

                vm->frames[vm->frame_count++] = (CallFrame){
                    .fn = fn,
                    .locals = locals,
                    .return_ip = vm->pos
                };

                vm->pos = fn->start_ip;

            } break;

            case OP_RET: {

                CallFrame *frame = current_frame(vm);

                if (!frame) error_invalid_opcode((ErrorLocation){0}, operation);

                Value ret = { .type = VAL_VOID };

                if (frame->fn->return_type != TOK_VOID_T) ret = pop(vm);

                free(frame->locals);
                vm->frame_count--;

                if (vm->frame_count == 0) return;

                vm->pos = frame->return_ip;

                if (frame->fn->return_type != TOK_VOID_T) push(vm, ret);

            } break;

            case OP_POP: {

                pop(vm);

            } break;

            case OP_JUMP: {

                uint16_t target = read_u16(vm);
                vm->pos = target;

            } break;

            case OP_JUMP_IF_FALSE: {

                uint16_t target = read_u16(vm);
                Value cond = pop(vm);

                if (cond.type == VAL_BOOLEAN && !cond.as.boolean) {

                    vm->pos = target;

                }

            } break;

            case OP_NOT: {

                Value v = pop(vm);
                
                if (v.type != VAL_BOOLEAN) error_invalid_opcode((ErrorLocation){0}, OP_NOT);
                
                push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = !v.as.boolean });

            } break;

            case OP_AND: {

                Value b = pop(vm);
                Value a = pop(vm);
                
                if (a.type != VAL_BOOLEAN || b.type != VAL_BOOLEAN) error_invalid_opcode((ErrorLocation){0}, OP_AND);
                
                push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.boolean && b.as.boolean });

            } break;

            case OP_OR: {

                Value b = pop(vm);
                Value a = pop(vm);
                
                if (a.type != VAL_BOOLEAN || b.type != VAL_BOOLEAN) error_invalid_opcode((ErrorLocation){0}, OP_OR);
                
                push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.boolean || b.as.boolean });

            } break;

            case OP_EQUAL: {

                Value b = pop(vm);
                Value a = pop(vm);
                bool result = false;

                if (a.type == b.type) {

                    if (a.type == VAL_INTEGER) result = a.as.integer == b.as.integer;
                    else if (a.type == VAL_FLOAT) result = a.as.floating == b.as.floating;
                    else if (a.type == VAL_BOOLEAN) result = a.as.boolean == b.as.boolean;
                    else if (a.type == VAL_STRING) result = strcmp(a.as.str, b.as.str) == 0;
                    
                } else {
                    
                    error_invalid_opcode((ErrorLocation){0}, OP_EQUAL);
                    
                }

                push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = result });

            } break;

            case OP_LESS: {

                Value b = pop(vm);
                Value a = pop(vm);
                
                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.integer < b.as.integer });
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.floating < b.as.floating });
                } else {
                    error_invalid_opcode((ErrorLocation){0}, OP_LESS);
                }

            } break;

            case OP_GREATER: {

                Value b = pop(vm);
                Value a = pop(vm);
                
                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.integer > b.as.integer });
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.floating > b.as.floating });
                } else {
                    error_invalid_opcode((ErrorLocation){0}, OP_GREATER);
                }

            } break;

            case OP_LESS_EQUAL: {

                Value b = pop(vm);
                Value a = pop(vm);
                
                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.integer <= b.as.integer });
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.floating <= b.as.floating });
                } else {
                    error_invalid_opcode((ErrorLocation){0}, OP_LESS_EQUAL);
                }

            } break;

            case OP_GREATER_EQUAL: {

                Value b = pop(vm);
                Value a = pop(vm);
                
                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.integer >= b.as.integer });
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    push(vm, (Value){ .type = VAL_BOOLEAN, .as.boolean = a.as.floating >= b.as.floating });
                } else {
                    error_invalid_opcode((ErrorLocation){0}, OP_GREATER_EQUAL);
                }

            } break;

            case OP_NEG: {

                Value v = pop(vm);
                
                if (v.type == VAL_INTEGER) {
                    push(vm, (Value){ .type = VAL_INTEGER, .as.integer = -v.as.integer });
                } else if (v.type == VAL_FLOAT) {
                    push(vm, (Value){ .type = VAL_FLOAT, .as.floating = -v.as.floating });
                } else {
                    error_invalid_opcode((ErrorLocation){0}, OP_NEG);
                }

            } break;

            case OP_ADD: {

                Value b = pop(vm);
                Value a = pop(vm);

                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {

                    push(vm, (Value){ .type = VAL_INTEGER, .as.integer = a.as.integer + b.as.integer });

                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {

                    push(vm, (Value){ .type = VAL_FLOAT, .as.floating = a.as.floating + b.as.floating });

                } else {

                    error_invalid_opcode((ErrorLocation){0}, OP_ADD);

                }

            } break;

            case OP_SUB: {

                Value b = pop(vm);
                Value a = pop(vm);

                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {

                    push(vm, (Value){ .type = VAL_INTEGER, .as.integer = a.as.integer - b.as.integer });

                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {

                    push(vm, (Value){ .type = VAL_FLOAT, .as.floating = a.as.floating - b.as.floating });

                } else {

                    error_invalid_opcode((ErrorLocation){0}, OP_SUB);

                }

            } break;

            case OP_MUL: {

                Value b = pop(vm);
                Value a = pop(vm);

                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {

                    push(vm, (Value){ .type = VAL_INTEGER, .as.integer = a.as.integer * b.as.integer });

                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {

                    push(vm, (Value){ .type = VAL_FLOAT, .as.floating = a.as.floating * b.as.floating });

                } else {

                    error_invalid_opcode((ErrorLocation){0}, OP_MUL);

                }

            } break;

            case OP_DIV: {

                Value b = pop(vm);
                Value a = pop(vm);

                if (a.type == VAL_INTEGER && b.type == VAL_INTEGER) {

                    push(vm, (Value){ .type = VAL_INTEGER, .as.integer = a.as.integer / b.as.integer });

                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {

                    push(vm, (Value){ .type = VAL_FLOAT, .as.floating = a.as.floating / b.as.floating });

                } else {

                    error_invalid_opcode((ErrorLocation){0}, OP_DIV);

                }

            } break;

            case OP_HALT: return; break;

            default: error_invalid_opcode((ErrorLocation){0}, operation);

        }

    }

}
