#include "lexer.c"

// Declarations
typedef enum {

    DEC_ENTRY,
    DEC_VAR

} DeclarationTag;

// Statements
typedef enum {

    STM_OUT,
    STM_ASSIGN,
    STM_VAR_DECL

} StatementTag;

// Expressions
typedef enum {

    EXP_STRING,
    EXP_INTEGER,
    EXP_VARIABLE

} ExpressionTag;

typedef struct {

    ExpressionTag tag;
    int line;

    union {

        struct { char *value; } str_lit;
        struct { int value; } int_lit;
        struct { char *name; } variable;

    };

} AstExpression;

typedef struct {

    StatementTag tag;
    int line;

    union {

        struct { AstExpression *expression; } out;
        struct { char *var_name; AstExpression *expression; } assign;
        struct { char *var_name; TokenType var_type; AstExpression *init_expr; } var_decl;

    };

} AstStatement;

// Block
typedef struct {

    AstStatement **statements;
    size_t len, cap;

} AstBlock;

typedef struct {

    DeclarationTag tag;
    int line;

    union {

        struct { AstBlock *block; } entry;
        struct { char *var_name; TokenType var_type; } var_decl;

    };

} AstDeclaration;

// Program
typedef struct {

    AstDeclaration **declarations;
    size_t len, cap;

} AstProgram;

typedef struct {

    Lexer *lexer;
    Token look;

} Parser;

static Parser init_parser(Lexer *lexer) {

    Parser parser = { .lexer = lexer, .look = next_token(lexer) };

    return parser;

}

static void vector_push(void ***items, size_t *len, size_t *cap, void *item) {

    if (*len + 1 > *cap) {

        size_t new_cap = *cap ? *cap * 2 : 8;
        void **new_items = realloc(*items, new_cap * sizeof(void*));

        if (!items) {

            error_oom();

        }

        *items = new_items;
        *cap = new_cap;

    }

    (*items)[(*len)++] = item;

}

static void free_token(Token *token) {

    // TODO: Fix the heap allocation and
    // detection system here so it isn't
    // garbage
    TokenType heap_tokens[] = {TOK_OUT, TOK_ENTRY, TOK_STRING_LIT, TOK_INTEGER_LIT, TOK_VARIABLE, TOK_STRING_T, TOK_INTEGER_T};
    size_t ht_len = sizeof(heap_tokens) / sizeof(heap_tokens[0]);
    bool needs_freeing = is_heap_lexeme(*token, heap_tokens, ht_len);

    if (token->lexeme && needs_freeing) free(token->lexeme);

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

static void expect(Parser *parser, TokenType t_type,const char *message) {

    if (!match(parser, t_type)) {

        error_expect_generic(parser->look.line, message);

    }

}

static AstExpression *parse_expression(Parser *parser) {

    if (parser->look.type == TOK_STRING_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        expression->tag = EXP_STRING;
        expression->line = parser->look.line;
        expression->str_lit.value = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        return expression;

    }

    if (parser->look.type == TOK_INTEGER_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        expression->tag = EXP_INTEGER;
        expression->line = parser->look.line;
        expression->int_lit.value = atoi(parser->look.lexeme ? parser->look.lexeme : "0");

        advance_parser(parser);

        return expression;

    }

    if (parser->look.type == TOK_VARIABLE) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        expression->tag = EXP_VARIABLE;
        expression->line = parser->look.line;
        expression->variable.name = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        return expression;

    }

    error_expect_expression(parser->look.line);

}

static AstStatement *parse_statement(Parser *parser) {

    if (parser->look.type == TOK_OUT) {

        int line = parser->look.line;
        advance_parser(parser);

        AstExpression *expression = parse_expression(parser);
        AstStatement *statement = calloc(1, sizeof(*statement));

        statement->tag = STM_OUT;
        statement->line = line;
        statement->out.expression = expression;

        return statement;

    }

    if (parser->look.type == TOK_VARIABLE) {

        int line = parser->look.line;
        char *var_name = strdup(parser->look.lexeme ? parser->look.lexeme : "");
        advance_parser(parser);

        expect(parser, TOK_ASSIGN, "'='");

        AstExpression *expression = parse_expression(parser);
        AstStatement *statement = calloc(1, sizeof(*statement));

        statement->tag = STM_ASSIGN;
        statement->line = line;
        statement->assign.var_name = var_name;
        statement->assign.expression = expression;

        return statement;

    }

    if (parser->look.type == TOK_INTEGER_T || parser->look.type == TOK_STRING_T) {

        int line = parser->look.line;
        TokenType var_type = parser->look.type;
        advance_parser(parser);

        if (parser->look.type != TOK_VARIABLE) {
            error_expect_generic(parser->look.line, "variable name");
        }

        char *var_name = strdup(parser->look.lexeme ? parser->look.lexeme : "");
        advance_parser(parser);

        AstExpression *init_expr = NULL;
        
        if (match(parser, TOK_ASSIGN)) {
            init_expr = parse_expression(parser);
        }

        AstStatement *statement = calloc(1, sizeof(*statement));
        statement->tag = STM_VAR_DECL;
        statement->line = line;
        statement->var_decl.var_name = var_name;
        statement->var_decl.var_type = var_type;
        statement->var_decl.init_expr = init_expr;

        return statement;

    }

    error_expect_statement(parser->look.line);

}

static AstBlock *parse_block(Parser *parser) {

    expect(parser, TOK_LBRACE, "'{'");
    AstBlock *block = calloc(1, sizeof(*block));

    while (parser->look.type != TOK_RBRACE) {

        AstStatement *statement = parse_statement(parser);
        vector_push((void***)&block->statements, &block->len, &block->cap, statement);

    }

    expect(parser, TOK_RBRACE, "'}'");

    return block;

}

static AstDeclaration *parse_entry_decl(Parser *parser) {

    int line = parser->look.line;
    expect(parser, TOK_ENTRY, "'entry'");

    AstBlock *block = parse_block(parser);
    AstDeclaration *declaration = calloc(1, sizeof(*declaration));

    declaration->tag = DEC_ENTRY;
    declaration->line = line;
    declaration->entry.block = block;

    return declaration;

}

static AstDeclaration *parse_var_decl(Parser *parser) {

    int line = parser->look.line;
    TokenType var_type = parser->look.type;
    advance_parser(parser);

    if (parser->look.type == TOK_VARIABLE) {

        char *var_name = strdup(parser->look.lexeme ? parser->look.lexeme : "");
        advance_parser(parser);

        AstDeclaration *declaration = calloc(1, sizeof(*declaration));

        declaration->tag = DEC_VAR;
        declaration->line = line;
        declaration->var_decl.var_name = var_name;
        declaration->var_decl.var_type = var_type;

        return declaration;

    } else {

        AstDeclaration *declaration = calloc(1, sizeof(*declaration));

        declaration->tag = DEC_VAR;
        declaration->line = line;
        declaration->var_decl.var_name = NULL;
        declaration->var_decl.var_type = var_type;

        return declaration;

    }

}

static AstProgram *parse_program(Parser *parser) {

    AstProgram *program = calloc(1, sizeof(*program));

    while (parser->look.type != TOK_EOF) {

        AstDeclaration *declare = NULL;

        if (parser->look.type == TOK_ENTRY) {

            declare = parse_entry_decl(parser);

        } else if (parser->look.type == TOK_INTEGER_T || parser->look.type == TOK_STRING_T) {

            declare = parse_var_decl(parser);

        } else {

            error_invalid_token(parser->look.line);

        }

        vector_push((void***)&program->declarations, &program->len, &program->cap, declare);

    }

    return program;

}

/* AST freeing */
static void free_expression(AstExpression *expression) {

    if (!expression) return;

    switch (expression->tag) {

        case EXP_STRING: free(expression->str_lit.value); break;
        case EXP_INTEGER: break;
        case EXP_VARIABLE: free(expression->variable.name); break;

    }

    free(expression);

}

static void free_statement(AstStatement *statement) {

    if (!statement) return;

    switch (statement->tag) {

        case STM_OUT: free_expression(statement->out.expression); break;
        case STM_ASSIGN: 
            free(statement->assign.var_name);
            free_expression(statement->assign.expression);
            break;
        case STM_VAR_DECL:
            free(statement->var_decl.var_name);
            if (statement->var_decl.init_expr) {
                free_expression(statement->var_decl.init_expr);
            }
            break;

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

        case DEC_ENTRY:

            free_block(declaration->entry.block);
            break;

        case DEC_VAR:

            if (declaration->var_decl.var_name) {
                free(declaration->var_decl.var_name);
            }
            break;

    }

    free(declaration);

}

static void free_program(AstProgram *program) {

    if (!program) return;

    for (size_t i = 0; i < program->len; i++) free_declaration(program->declarations[i]);

    free(program->declarations);
    free(program);

}
