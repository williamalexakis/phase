#include "lexer.c"

typedef struct AstProgram AstProgram;
typedef struct AstDeclare AstDeclare;
typedef struct AstBlock AstBlock;
typedef struct AstStatement AstStatement;
typedef struct AstExpression AstExpression;

// Program
struct AstProgram {

    AstDeclare **decls;
    size_t len, cap;

};

// Declarations
typedef enum {

    D_ENTRY

} DeclareTag;

struct AstDeclare {

    DeclareTag tag;
    int line;

    union {

        struct { AstBlock *block; } entry;

    };

};

// Block
struct AstBlock {

    AstStatement **statements;
    size_t len, cap;

};

// Statements
typedef enum {

    S_OUT

} StatementTag;

struct AstStatement {

    StatementTag tag;
    int line;

    union {

        struct { AstExpression *expression; } out;

    };

};

// Expressions
typedef enum {

    E_STRING

} ExpressionTag;

struct AstExpression {

    ExpressionTag tag;
    int line;

    union {

        struct { char *value; } str_lit;

    };

};

static void vector_push(void ***items, size_t *len, size_t *cap, void *elt) {

    if (*len + 1 > *cap) {

        *cap = *cap ? *cap * 2 : 8;
        *items = realloc(*items, *cap * sizeof(void*));

        if (!items) {

            fprintf(stderr, "[phasec] ERROR: OUT OF MEMORY\n");
            free(items);

            exit(1);

        };

    }

    (*items)[(*len)++] = elt;

}

typedef struct {

    Lexer *lexer;
    Token look;

} Parser;

static Parser init_parser(Lexer *lexer) {

    Parser parser = { .lexer = lexer, .look = next_token(lexer) };

    return parser;

}

static void free_token(Token *token) {

    TokenType heap_tokens[3] = {T_OUT, T_ENTRY, T_STRING};
    size_t ht_len = sizeof(heap_tokens) / sizeof(heap_tokens[0]);
    bool needs_freeing = is_heap_lexeme(*token, heap_tokens, ht_len);

    if (token->lexeme && needs_freeing) free(token->lexeme);
    token->lexeme = NULL;

}

static void advance_parser(Parser *parser) {

    if (parser->look.type != T_EOF) free_token(&parser->look);

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

        fprintf(stderr, "[phasec] ERROR (LINE %d): EXPECTED %s\n", parser->look.line, message);
        exit(1);

    }

}

static AstExpression *parse_expression(Parser *parser) {

    if (parser->look.type == T_STRING) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        expression->tag = E_STRING;
        expression->line = parser->look.line;
        expression->str_lit.value = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        return expression;

    }

    fprintf(stderr, "[phasec] ERROR (LINE %d): EXPECTED EXPRESSION\n", parser->look.line);
    exit(1);

}

static AstStatement *parse_statement(Parser *parser) {

    if (parser->look.type == T_OUT) {

        int line = parser->look.line;
        advance_parser(parser);

        AstExpression *expression = parse_expression(parser);
        AstStatement *statement = calloc(1, sizeof(*statement));

        statement->tag = S_OUT;
        statement->line = line;
        statement->out.expression = expression;

        return statement;

    }

    fprintf(stderr, "[phasec] ERROR (LINE %d): EXPECTED STATEMENT\n", parser->look.line);
    exit(1);

}

static AstBlock *parse_block(Parser *parser) {

    expect(parser, T_LBRACE, "'{'");
    AstBlock *block = calloc(1, sizeof(*block));

    while (parser->look.type != T_RBRACE) {

        AstStatement *statement = parse_statement(parser);
        vector_push((void***)&block->statements, &block->len, &block->cap, statement);

    }

    expect(parser, T_RBRACE, "'}'");

    return block;

}

static AstDeclare *parse_entry_decl(Parser *parser) {

    int line = parser->look.line;
    expect(parser, T_ENTRY, "'entry'");

    AstBlock *block = parse_block(parser);
    AstDeclare *declare = calloc(1, sizeof(*declare));

    declare->tag = D_ENTRY;
    declare->line = line;
    declare->entry.block = block;

    return declare;

}

static AstProgram *parse_program(Parser *parser) {

    AstProgram *program = calloc(1, sizeof(*program));

    while (parser->look.type != T_EOF) {

        AstDeclare *declare = NULL;

        if (parser->look.type == T_ENTRY) {

            declare = parse_entry_decl(parser);

        } else {

            fprintf(stderr, "[phasec] ERROR (LINE %d): UNEXPECTED TOPLEVEL TOKEN\n", parser->look.line);
            exit(1);

        }

        vector_push((void***)&program->decls, &program->len, &program->cap, declare);

    }

    return program;

}

static void free_expression(AstExpression *expression) {

    if (!expression) return;

    switch (expression->tag) {

        case E_STRING: free(expression->str_lit.value); break;

    }

    free(expression);

}

static void free_statement(AstStatement *statement) {

    if (!statement) return;

    switch (statement->tag) {

        case S_OUT: free_expression(statement->out.expression); break;

    }

    free(statement);

}

static void free_block(AstBlock *block) {

    if (!block) return;

    for (size_t i = 0; i < block->len; i++) free_statement(block->statements[i]);

    free(block->statements);
    free(block);

}

static void free_declaration(AstDeclare *declare) {

    if (!declare) return;

    switch (declare->tag) {

        case D_ENTRY: free_block(declare->entry.block); break;

    }

    free(declare);

}

static void free_program(AstProgram *program) {

    if (!program) return;

    for (size_t i = 0; i < program->len; i++);

    free(program->decls);
    free(program);

}
