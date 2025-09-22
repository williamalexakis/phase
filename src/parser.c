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
        struct {
            char **var_names;
            size_t var_count;
            TokenType var_type;
            AstExpression **init_exprs;
            size_t init_count;
        } var_decl;

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
        struct {
            char **var_names;
            size_t var_count;
            TokenType var_type;
        } var_decl;

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

        if (!items) error_oom();

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

    // String literals
    if (parser->look.type == TOK_STRING_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_STRING;
        expression->line = parser->look.line;
        expression->str_lit.value = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        return expression;

    }

    // Integer literals
    if (parser->look.type == TOK_INTEGER_LIT) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_INTEGER;
        expression->line = parser->look.line;
        expression->int_lit.value = atoi(parser->look.lexeme ? parser->look.lexeme : "0");

        advance_parser(parser);

        return expression;

    }

    // Variable names
    if (parser->look.type == TOK_VARIABLE) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        if (!expression) error_oom();

        expression->tag = EXP_VARIABLE;
        expression->line = parser->look.line;
        expression->variable.name = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        return expression;

    }

    if (parser->look.type == TOK_LPAREN) {

        advance_parser(parser);
        AstExpression *expression = parse_expression(parser);
        expect(parser, TOK_RPAREN, "')'");

        return expression;

    }

    error_expect_expression(parser->look.line);

}

static AstStatement *parse_statement(Parser *parser) {

    if (parser->look.type == TOK_OUT) {

        int line = parser->look.line;
        advance_parser(parser);

        expect(parser, TOK_LPAREN, "'('");
        AstExpression *expression = parse_expression(parser);
        expect(parser, TOK_RPAREN, "')'");

        AstStatement *statement = calloc(1, sizeof(*statement));

        if (!statement) error_oom();

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

        if (!statement) error_oom();

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

        // Parse variable names (either single or grouped in parentheses)
        char **var_names = NULL;
        size_t var_count = 0;
        size_t var_cap = 0;

        if (parser->look.type == TOK_LPAREN) {
            // Grouped declaration: int (a, b, c)
            advance_parser(parser); // consume '('

            do {
                if (parser->look.type != TOK_VARIABLE) {
                    error_expect_generic(parser->look.line, "variable name");
                }

                // Expand array if needed
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
        } else if (parser->look.type == TOK_VARIABLE) {
            // Single declaration: int a
            var_names = malloc(sizeof(char*));
            if (!var_names) error_oom();
            var_names[0] = strdup(parser->look.lexeme ? parser->look.lexeme : "");
            var_count = 1;
            var_cap = 1;
            advance_parser(parser);
        } else {
            error_expect_generic(parser->look.line, "variable name or '('");
        }

        // Parse initialization expressions
        AstExpression **init_exprs = NULL;
        size_t init_count = 0;

        if (match(parser, TOK_ASSIGN)) {
            if (parser->look.type == TOK_LPAREN) {
                // Grouped initialization: = (5, 10, 15)
                advance_parser(parser); // consume '('

                size_t init_cap = 0;
                do {
                    // Expand array if needed
                    if (init_count + 1 > init_cap) {
                        size_t new_cap = init_cap ? init_cap * 2 : 4;
                        init_exprs = realloc(init_exprs, new_cap * sizeof(AstExpression*));
                        if (!init_exprs) error_oom();
                        init_cap = new_cap;
                    }

                    init_exprs[init_count++] = parse_expression(parser);

                } while (match(parser, TOK_COMMA));

                expect(parser, TOK_RPAREN, "')'");
            } else {
                // Single initialization: = 5
                init_exprs = malloc(sizeof(AstExpression*));
                if (!init_exprs) error_oom();
                init_exprs[0] = parse_expression(parser);
                init_count = 1;
            }
        }

        AstStatement *statement = calloc(1, sizeof(*statement));
        statement->tag = STM_VAR_DECL;
        statement->line = line;
        statement->var_decl.var_names = var_names;
        statement->var_decl.var_count = var_count;
        statement->var_decl.var_type = var_type;
        statement->var_decl.init_exprs = init_exprs;
        statement->var_decl.init_count = init_count;

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

    // Parse variable names (either single or grouped in parentheses)
    char **var_names = NULL;
    size_t var_count = 0;
    size_t var_cap = 0;

    if (parser->look.type == TOK_LPAREN) {
        // Grouped declaration: int (a, b, c)
        advance_parser(parser); // consume '('

        do {
            if (parser->look.type != TOK_VARIABLE) {
                error_expect_generic(parser->look.line, "variable name");
            }

            // Expand array if needed
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
    } else if (parser->look.type == TOK_VARIABLE) {
        // Single declaration: int a
        var_names = malloc(sizeof(char*));
        if (!var_names) error_oom();
        var_names[0] = strdup(parser->look.lexeme ? parser->look.lexeme : "");
        var_count = 1;
        var_cap = 1;
        advance_parser(parser);
    } else {
        // Anonymous declaration: just int or str
        var_names = NULL;
        var_count = 0;
    }

    AstDeclaration *declaration = calloc(1, sizeof(*declaration));
    declaration->tag = DEC_VAR;
    declaration->line = line;
    declaration->var_decl.var_names = var_names;
    declaration->var_decl.var_count = var_count;
    declaration->var_decl.var_type = var_type;

    return declaration;

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
            for (size_t i = 0; i < statement->var_decl.var_count; i++) {
                free(statement->var_decl.var_names[i]);
            }
            free(statement->var_decl.var_names);
            for (size_t i = 0; i < statement->var_decl.init_count; i++) {
                free_expression(statement->var_decl.init_exprs[i]);
            }
            free(statement->var_decl.init_exprs);
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

            for (size_t i = 0; i < declaration->var_decl.var_count; i++) {
                free(declaration->var_decl.var_names[i]);
            }
            free(declaration->var_decl.var_names);
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
