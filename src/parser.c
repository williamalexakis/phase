#include <stdlib.h>
#include <string.h>
#include "parser.h"

Parser init_parser(Lexer *lexer) {

    Parser parser = { .lexer = lexer, .look = next_token(lexer) };

    return parser;

}

static void vector_push(void ***items, size_t *len, size_t *cap, void *item) {

    if (*len + 1 > *cap) {

        size_t new_cap = *cap ? *cap * 2 : 8;
        void **new_items = realloc(*items, new_cap * sizeof(void*));

        if (!new_items) error_oom();

        *items = new_items;
        *cap = new_cap;

    }

    (*items)[(*len)++] = item;

}

static AstBlock *parse_block(Parser *parser);
static AstExpression *parse_expression(Parser *parser);
static AstStatement *parse_statement(Parser *parser);
static void free_block(AstBlock *block);

void free_token(Token *token) {

    if (token->lexeme && token->heap_allocated) free(token->lexeme);

}

static void advance_parser(Parser *parser) {

    if (parser->look.type != TOK_EOF) free_token(&parser->look);

    parser->look = next_token(parser->lexer);

}


static bool match(Parser *parser, TokenType t_type) {

    if (parser->look.type == t_type) {

        advance_parser(parser);

        return true;

    }

    return false;

}

/* Expect a specific token type and error if not found */
static void expect(Parser *parser, TokenType t_type, const char *message) {

    if (!match(parser, t_type)) {

        ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
        error_expect_symbol(loc, message);

    }

}

static TokenType parse_type_annotation(Parser *parser, bool allow_void, int *col_end_out) {

    if (parser->look.type == TOK_INTEGER_T
        || parser->look.type == TOK_STRING_T
        || parser->look.type == TOK_FLOAT_T
        || parser->look.type == TOK_BOOLEAN_T
        || (allow_void && parser->look.type == TOK_VOID_T)) {

        TokenType var_type = parser->look.type;

        if (col_end_out) *col_end_out = parser->look.column_end;

        advance_parser(parser);

        return var_type;

    }

    ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
    error_expect_symbol(loc, "type name");

    return TOK_UNKNOWN;

}

static AstExpression *parse_expression(Parser *parser) {

    if (parser->look.type == TOK_STRING_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_STRING;
        expression->line = parser->look.line;
        expression->column_start = parser->look.column_start;
        expression->column_end = parser->look.column_end;
        expression->str_lit.value = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        return expression;

    }

    if (parser->look.type == TOK_INTEGER_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_INTEGER;
        expression->line = parser->look.line;
        expression->column_start = parser->look.column_start;
        expression->column_end = parser->look.column_end;
        expression->int_lit.value = atoi(parser->look.lexeme ? parser->look.lexeme : "0");

        advance_parser(parser);

        return expression;

    }

    if (parser->look.type == TOK_FLOAT_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_FLOAT;
        expression->line = parser->look.line;
        expression->column_start = parser->look.column_start;
        expression->column_end = parser->look.column_end;
        expression->float_lit.value = atof(parser->look.lexeme ? parser->look.lexeme : "0.0");

        advance_parser(parser);

        return expression;

    }

    if (parser->look.type == TOK_BOOLEAN_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_BOOLEAN;
        expression->line = parser->look.line;
        expression->column_start = parser->look.column_start;
        expression->column_end = parser->look.column_end;
        expression->bool_lit.value = strcmp(parser->look.lexeme, "true") == 0;

        advance_parser(parser);

        return expression;

    }

    if (parser->look.type == TOK_VARIABLE) {

        int line = parser->look.line;
        int col_start = parser->look.column_start;
        int name_col_end = parser->look.column_end;
        char *name = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        if (parser->look.type == TOK_LPAREN) {

            advance_parser(parser);

            AstExpression **args = NULL;
            size_t arg_count = 0;
            size_t arg_cap = 0;

            if (parser->look.type != TOK_RPAREN) {

                do {

                    if (arg_count + 1 > arg_cap) {

                        size_t new_cap = arg_cap ? arg_cap * 2 : 4;
                        args = realloc(args, new_cap * sizeof(AstExpression*));

                        if (!args) error_oom();

                        arg_cap = new_cap;

                    }

                    args[arg_count++] = parse_expression(parser);

                } while (match(parser, TOK_COMMA));

            }

            if (parser->look.type != TOK_RPAREN) {

                ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
                error_expect_symbol(loc, "')'");

            }

            int col_end = parser->look.column_end;
            expect(parser, TOK_RPAREN, "')'");

            AstExpression *expression = calloc(1, sizeof(*expression));

            if (!expression) error_oom();

            expression->tag = EXP_CALL;
            expression->line = line;
            expression->column_start = col_start;
            expression->column_end = col_end;
            expression->call.func_name = name;
            expression->call.args = args;
            expression->call.arg_count = arg_count;

            return expression;

        }

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_VARIABLE;
        expression->line = line;
        expression->column_start = col_start;
        expression->column_end = name_col_end;
        expression->variable.name = name;

        return expression;

    }

    if (parser->look.type == TOK_LPAREN) {

        advance_parser(parser);
        AstExpression *expression = parse_expression(parser);
        expect(parser, TOK_RPAREN, "')'");

        return expression;

    }

    ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
    error_expect_symbol(loc, "expression");
    
    return NULL;

}

static AstStatement *parse_statement(Parser *parser) {

    if (parser->look.type == TOK_OUT) {

        int line = parser->look.line;
        int col_start = parser->look.column_start;
        int col_end = parser->look.column_end;

        advance_parser(parser);

        expect(parser, TOK_LPAREN, "'('");
        AstExpression *expression = parse_expression(parser);
        expect(parser, TOK_RPAREN, "')'");

        AstStatement *statement = calloc(1, sizeof(*statement));

        if (!statement) error_oom();

        statement->tag = STM_OUT;
        statement->line = line;
        statement->column_start = col_start;
        statement->column_end = col_end;
        statement->out.expression = expression;

        return statement;

    }

    if (parser->look.type == TOK_IF) {

        int line = parser->look.line;
        int col_start = parser->look.column_start;

        advance_parser(parser);

        AstExpression *condition = parse_expression(parser);
        AstBlock *then_block = parse_block(parser);

        AstBlock *else_block = NULL;

        if (parser->look.type == TOK_ELSE) {

            advance_parser(parser);

            if (parser->look.type == TOK_IF) {

                AstStatement *nested_if = parse_statement(parser);

                else_block = calloc(1, sizeof(*else_block));

                if (!else_block) error_oom();

                else_block->statements = malloc(sizeof(AstStatement*));

                if (!else_block->statements) error_oom();

                else_block->statements[0] = nested_if;
                else_block->len = 1;
                else_block->cap = 1;

            } else {

                else_block = parse_block(parser);

            }

        }

        int col_end = then_block ? then_block->len > 0 ? then_block->statements[then_block->len - 1]->column_end : col_start : col_start;

        AstStatement *statement = calloc(1, sizeof(*statement));

        if (!statement) error_oom();

        statement->tag = STM_IF;
        statement->line = line;
        statement->column_start = col_start;
        statement->column_end = col_end;
        statement->if_stmt.condition = condition;
        statement->if_stmt.then_block = then_block;
        statement->if_stmt.else_block = else_block;

        return statement;

    }

    if (parser->look.type == TOK_WHILE) {

        int line = parser->look.line;
        int col_start = parser->look.column_start;

        advance_parser(parser);

        AstExpression *condition = parse_expression(parser);
        AstBlock *body = parse_block(parser);

        int col_end = body && body->len > 0 ? body->statements[body->len - 1]->column_end : col_start;

        AstStatement *statement = calloc(1, sizeof(*statement));

        if (!statement) error_oom();

        statement->tag = STM_WHILE;
        statement->line = line;
        statement->column_start = col_start;
        statement->column_end = col_end;
        statement->if_stmt.condition = condition;
        statement->if_stmt.then_block = body;
        statement->if_stmt.else_block = NULL;

        return statement;

    }

    if (parser->look.type == TOK_RETURN) {

        int line = parser->look.line;
        int col_start = parser->look.column_start;
        int col_end = parser->look.column_end;

        advance_parser(parser);

        AstExpression *expression = NULL;

        if (parser->look.type != TOK_NEWLINE && parser->look.type != TOK_RBRACE && parser->look.type != TOK_EOF) {

            expression = parse_expression(parser);
            col_end = expression->column_end;

        }

        AstStatement *statement = calloc(1, sizeof(*statement));

        if (!statement) error_oom();

        statement->tag = STM_RETURN;
        statement->line = line;
        statement->column_start = col_start;
        statement->column_end = col_end;
        statement->ret.expression = expression;

        return statement;

    }

    if (parser->look.type == TOK_VARIABLE) {

        int line = parser->look.line;
        int col_start = parser->look.column_start;
        int col_end = parser->look.column_end;
        char *var_name = strdup(parser->look.lexeme ? parser->look.lexeme : "");
        advance_parser(parser);

        if (parser->look.type == TOK_LPAREN) {

            // Treat as call expression statement
            advance_parser(parser);

            AstExpression **args = NULL;
            size_t arg_count = 0;
            size_t arg_cap = 0;

            if (parser->look.type != TOK_RPAREN) {

                do {

                    if (arg_count + 1 > arg_cap) {

                        size_t new_cap = arg_cap ? arg_cap * 2 : 4;
                        args = realloc(args, new_cap * sizeof(AstExpression*));

                        if (!args) error_oom();

                        arg_cap = new_cap;

                    }

                    args[arg_count++] = parse_expression(parser);

                } while (match(parser, TOK_COMMA));

            }

            if (parser->look.type != TOK_RPAREN) {

                ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
                error_expect_symbol(loc, "')'");

            }

            col_end = parser->look.column_end;
            expect(parser, TOK_RPAREN, "')'");

            AstExpression *expr = calloc(1, sizeof(*expr));

            if (!expr) error_oom();

            expr->tag = EXP_CALL;
            expr->line = line;
            expr->column_start = col_start;
            expr->column_end = col_end;
            expr->call.func_name = var_name;
            expr->call.args = args;
            expr->call.arg_count = arg_count;

            AstStatement *statement = calloc(1, sizeof(*statement));

            if (!statement) error_oom();

            statement->tag = STM_EXPR;
            statement->line = line;
            statement->column_start = col_start;
            statement->column_end = col_end;
            statement->expr.expression = expr;

            return statement;

        }

        if (parser->look.type != TOK_ASSIGN) {

            ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };

            if (parser->look.type == TOK_COLON) {

                error_expect_symbol(loc, "assignment '=' (use 'let' for declarations)");

            } else {

                error_unexpected_ident(loc, var_name);

            }

        }

        advance_parser(parser);

        AstExpression *expression = parse_expression(parser);
        AstStatement *statement = calloc(1, sizeof(*statement));

        if (!statement) error_oom();

        statement->tag = STM_ASSIGN;
        statement->line = line;
        statement->column_start = col_start;
        statement->column_end = col_end;
        statement->assign.var_name = var_name;
        statement->assign.expression = expression;

        return statement;

    }

    if (parser->look.type == TOK_LET) {

        int line = parser->look.line;
        int col_start = parser->look.column_start;
        int col_end = parser->look.column_end;

        advance_parser(parser);

        char **var_names = NULL;
        size_t var_count = 0;
        size_t var_cap = 0;

        // Grouped declaration
        if (parser->look.type == TOK_LPAREN) {

            advance_parser(parser);

            do {

                if (parser->look.type != TOK_VARIABLE) {

                    ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
                    error_expect_symbol(loc, "variable name");

                }

                if (var_count + 1 > var_cap) {

                    size_t new_cap = var_cap ? var_cap * 2 : 4;
                    var_names = realloc(var_names, new_cap * sizeof(char*));

                    if (!var_names) error_oom();

                    var_cap = new_cap;

                }

                var_names[var_count++] = strdup(parser->look.lexeme ? parser->look.lexeme : "");
                advance_parser(parser);

            } while (match(parser, TOK_COMMA));

            expect(parser, TOK_RPAREN, "')'");

        // Single declaration
        } else if (parser->look.type == TOK_VARIABLE) {

            var_names = malloc(sizeof(char*));

            if (!var_names) error_oom();

            var_names[0] = strdup(parser->look.lexeme ? parser->look.lexeme : "");
            var_count = 1;
            var_cap = 1;

            advance_parser(parser);

        } else {

            ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
            error_expect_symbol(loc, "variable name or '('");

        }

        expect(parser, TOK_COLON, "':'");

        TokenType var_type = parse_type_annotation(parser, false, &col_end);

        AstExpression **init_exprs = NULL;
        size_t init_count = 0;

        if (match(parser, TOK_ASSIGN)) {

            // Grouped initialization
            if (parser->look.type == TOK_LPAREN) {

                advance_parser(parser);

                size_t init_cap = 0;

                do {

                    if (init_count + 1 > init_cap) {

                        size_t new_cap = init_cap ? init_cap * 2 : 4;
                        init_exprs = realloc(init_exprs, new_cap * sizeof(AstExpression*));

                        if (!init_exprs) error_oom();

                        init_cap = new_cap;

                    }

                    init_exprs[init_count++] = parse_expression(parser);

                } while (match(parser, TOK_COMMA));

                expect(parser, TOK_RPAREN, "')'");

            // Single initialization
            } else {

                init_exprs = malloc(sizeof(AstExpression*));

                if (!init_exprs) error_oom();

                init_exprs[0] = parse_expression(parser);
                init_count = 1;

            }

        }

        AstStatement *statement = calloc(1, sizeof(*statement));

        statement->tag = STM_VAR_DECL;
        statement->line = line;
        statement->column_start = col_start;
        statement->column_end = col_end;
        statement->var_decl.var_names = var_names;
        statement->var_decl.var_count = var_count;
        statement->var_decl.var_type = var_type;
        statement->var_decl.init_exprs = init_exprs;
        statement->var_decl.init_count = init_count;

        return statement;

    }

    ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
    error_expect_symbol(loc, "statement or declaration");
    
    return NULL;

}

static AstBlock *parse_block(Parser *parser) {

    expect(parser, TOK_LBRACE, "'{'");
    AstBlock *block = calloc(1, sizeof(*block));

    while (parser->look.type == TOK_NEWLINE) advance_parser(parser);

    while (parser->look.type != TOK_RBRACE) {

        AstStatement *statement = parse_statement(parser);
        vector_push((void***)&block->statements, &block->len, &block->cap, statement);

        if (parser->look.type != TOK_RBRACE) {

            if (parser->look.type == TOK_NEWLINE) {

                advance_parser(parser);

                while (parser->look.type == TOK_NEWLINE) advance_parser(parser);

            } else {

                ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
                error_expect_symbol(loc, "newline or end of block");

            }

        }

    }

    expect(parser, TOK_RBRACE, "'}'");

    return block;

}

static AstDeclaration *parse_entry_decl(Parser *parser) {

    int line = parser->look.line;
    int col_start = parser->look.column_start;
    int col_end = parser->look.column_end;

    expect(parser, TOK_ENTRY, "'entry'");

    AstBlock *block = parse_block(parser);
    AstDeclaration *declaration = calloc(1, sizeof(*declaration));

    declaration->tag = DEC_ENTRY;
    declaration->line = line;
    declaration->column_start = col_start;
    declaration->column_end = col_end;
    declaration->entry.block = block;

    return declaration;

}

static AstDeclaration *parse_func_decl(Parser *parser) {

    int line = parser->look.line;
    int col_start = parser->look.column_start;
    int col_end = parser->look.column_end;

    expect(parser, TOK_FUNC, "'func'");

    if (parser->look.type != TOK_VARIABLE) {

        ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
        error_expect_symbol(loc, "function name");

    }

    char *name = strdup(parser->look.lexeme ? parser->look.lexeme : "");
    advance_parser(parser);

    expect(parser, TOK_LPAREN, "'('");

    AstParam *params = NULL;
    size_t param_count = 0;
    size_t param_cap = 0;

    if (parser->look.type != TOK_RPAREN) {

        do {

            if (parser->look.type != TOK_VARIABLE) {

                ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
                error_expect_symbol(loc, "parameter name");

            }

            if (param_count + 1 > param_cap) {

                size_t new_cap = param_cap ? param_cap * 2 : 4;
                params = realloc(params, new_cap * sizeof(AstParam));

                if (!params) error_oom();

                param_cap = new_cap;

            }

            params[param_count].name = strdup(parser->look.lexeme ? parser->look.lexeme : "");
            params[param_count].line = parser->look.line;
            params[param_count].column_start = parser->look.column_start;
            params[param_count].column_end = parser->look.column_end;

            advance_parser(parser);

            expect(parser, TOK_COLON, "':'");

            params[param_count].type = parse_type_annotation(parser, false, &params[param_count].column_end);

            param_count++;

        } while (match(parser, TOK_COMMA));

    }

    expect(parser, TOK_RPAREN, "')'");
    expect(parser, TOK_COLON, "':'");

    TokenType return_type = parse_type_annotation(parser, true, &col_end);

    AstBlock *body = parse_block(parser);

    AstDeclaration *declaration = calloc(1, sizeof(*declaration));

    if (!declaration) error_oom();

    declaration->tag = DEC_FUNC;
    declaration->line = line;
    declaration->column_start = col_start;
    declaration->column_end = col_end;
    declaration->func.name = name;
    declaration->func.params = params;
    declaration->func.param_count = param_count;
    declaration->func.return_type = return_type;
    declaration->func.body = body;

    return declaration;

}

static AstDeclaration *parse_var_decl(Parser *parser) {

    int line = parser->look.line;
    int col_start = parser->look.column_start;
    int col_end = parser->look.column_end;

    expect(parser, TOK_LET, "'let'");

    char **var_names = NULL;
    size_t var_count = 0;
    size_t var_cap = 0;

    // Grouped declaration
    if (parser->look.type == TOK_LPAREN) {

        advance_parser(parser);

        do {

            if (parser->look.type != TOK_VARIABLE) {

                ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
                error_expect_symbol(loc, "variable name");

            }

            if (var_count + 1 > var_cap) {

                size_t new_cap = var_cap ? var_cap * 2 : 4;
                var_names = realloc(var_names, new_cap * sizeof(char*));

                if (!var_names) error_oom();

                var_cap = new_cap;

            }

            var_names[var_count++] = strdup(parser->look.lexeme ? parser->look.lexeme : "");
            advance_parser(parser);

        } while (match(parser, TOK_COMMA));

        expect(parser, TOK_RPAREN, "')'");

    // Single declaration
    } else if (parser->look.type == TOK_VARIABLE) {

        var_names = malloc(sizeof(char*));

        if (!var_names) error_oom();

        var_names[0] = strdup(parser->look.lexeme ? parser->look.lexeme : "");
        var_count = 1;
        var_cap = 1;

        advance_parser(parser);

    } else {

        ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
        error_expect_symbol(loc, "variable name or '('");

    }

    expect(parser, TOK_COLON, "':'");

    TokenType var_type = parse_type_annotation(parser, false, &col_end);

    AstDeclaration *declaration = calloc(1, sizeof(*declaration));

    declaration->tag = DEC_VAR;
    declaration->line = line;
    declaration->column_start = col_start;
    declaration->column_end = col_end;
    declaration->var_decl.var_names = var_names;
    declaration->var_decl.var_count = var_count;
    declaration->var_decl.var_type = var_type;

    return declaration;

}

AstProgram *parse_program(Parser *parser) {

    AstProgram *program = calloc(1, sizeof(*program));

    while (parser->look.type == TOK_NEWLINE) advance_parser(parser);

    while (parser->look.type != TOK_EOF) {

        while (parser->look.type == TOK_NEWLINE) advance_parser(parser);

        if (parser->look.type == TOK_EOF) break;

        AstDeclaration *declaration = NULL;

        if (parser->look.type == TOK_ENTRY) {

            declaration = parse_entry_decl(parser);

        } else if (parser->look.type == TOK_LET) {

            declaration = parse_var_decl(parser);

        } else if (parser->look.type == TOK_FUNC) {

            declaration = parse_func_decl(parser);

        } else {

            ErrorLocation loc = { .file = parser->lexer->file_path, .line = parser->look.line, .col_start = parser->look.column_start, .col_end = parser->look.column_end };
            error_invalid_token(loc);

        }

        vector_push((void***)&program->declarations, &program->len, &program->cap, declaration);

        while (parser->look.type == TOK_NEWLINE) advance_parser(parser);

    }

    return program;

}

static void free_expression(AstExpression *expression) {

    if (!expression) return;

    switch (expression->tag) {

        case EXP_STRING: free(expression->str_lit.value); break;
        case EXP_INTEGER: break;
        case EXP_FLOAT: break;
        case EXP_BOOLEAN: break;
        case EXP_VARIABLE: free(expression->variable.name); break;
        case EXP_CALL: {

            for (size_t i = 0; i < expression->call.arg_count; i++) {

                free_expression(expression->call.args[i]);

            }

            free(expression->call.args);
            free(expression->call.func_name);

        } break;

    }

    free(expression);

}

static void free_statement(AstStatement *statement) {

    if (!statement) return;

    switch (statement->tag) {

        case STM_OUT: free_expression(statement->out.expression); break;

        case STM_ASSIGN: {

            free(statement->assign.var_name);
            free_expression(statement->assign.expression);

        } break;

        case STM_VAR_DECL: {

            for (size_t i = 0; i < statement->var_decl.var_count; i++) {

                free(statement->var_decl.var_names[i]);

            }

            free(statement->var_decl.var_names);

            for (size_t i = 0; i < statement->var_decl.init_count; i++) {

                free_expression(statement->var_decl.init_exprs[i]);

            }

            free(statement->var_decl.init_exprs);

        } break;

        case STM_RETURN: free_expression(statement->ret.expression); break;
        case STM_EXPR: free_expression(statement->expr.expression); break;
        case STM_IF: {

            free_expression(statement->if_stmt.condition);
            free_block(statement->if_stmt.then_block);
            free_block(statement->if_stmt.else_block);

        } break;
        case STM_WHILE: {

            free_expression(statement->if_stmt.condition);
            free_block(statement->if_stmt.then_block);

        } break;

    }

    free(statement);

}

static void free_block(AstBlock *block) {

    if (!block) return;

    for (size_t i = 0; i < block->len; i++) free_statement(block->statements[i]);

    free(block->statements);
    free(block);

}

static void free_declaration(AstDeclaration *declaration) {

    if (!declaration) return;

    switch (declaration->tag) {

        case DEC_ENTRY: free_block(declaration->entry.block); break;

        case DEC_VAR: {

            for (size_t i = 0; i < declaration->var_decl.var_count; i++) {

                free(declaration->var_decl.var_names[i]);

            }

            free(declaration->var_decl.var_names);

        } break;
        case DEC_FUNC: {

            free(declaration->func.name);

            for (size_t i = 0; i < declaration->func.param_count; i++) {

                free(declaration->func.params[i].name);

            }

            free(declaration->func.params);
            free_block(declaration->func.body);

        } break;

    }

    free(declaration);

}

void free_program(AstProgram *program) {

    if (!program) return;

    for (size_t i = 0; i < program->len; i++) free_declaration(program->declarations[i]);

    free(program->declarations);
    free(program);

}
