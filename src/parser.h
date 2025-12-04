#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include "lexer.h"

typedef enum {

    DEC_ENTRY,
    DEC_VAR,
    DEC_FUNC

} DeclarationTag;

typedef enum {

    STM_OUT,
    STM_ASSIGN,
    STM_VAR_DECL,
    STM_RETURN,
    STM_EXPR

} StatementTag;

typedef enum {

    EXP_STRING,
    EXP_INTEGER,
    EXP_FLOAT,
    EXP_BOOLEAN,
    EXP_VARIABLE,
    EXP_CALL

} ExpressionTag;

typedef struct AstExpression {

    ExpressionTag tag;
    int line;
    int column_start;
    int column_end;

    union {

        struct { char *value; } str_lit;
        struct { int value; } int_lit;
        struct { float value; } float_lit;
        struct { bool value; } bool_lit;
        struct { char *name; } variable;
        struct {
            char *func_name;
            struct AstExpression **args;
            size_t arg_count;
        } call;

    };

} AstExpression;

typedef struct {

    char *name;
    TokenType type;
    int line;
    int column_start;
    int column_end;

} AstParam;

typedef struct {

    StatementTag tag;
    int line;
    int column_start;
    int column_end;

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
        struct { AstExpression *expression; } ret;
        struct { AstExpression *expression; } expr;

    };

} AstStatement;

typedef struct {

    AstStatement **statements;
    size_t len, cap;

} AstBlock;

typedef struct {

    DeclarationTag tag;
    int line;
    int column_start;
    int column_end;

    union {

        struct { AstBlock *block; } entry;
        struct {

            char **var_names;
            size_t var_count;
            TokenType var_type;

        } var_decl;
        struct {

            char *name;
            AstParam *params;
            size_t param_count;
            TokenType return_type;
            AstBlock *body;

        } func;

    };

} AstDeclaration;

typedef struct {

    AstDeclaration **declarations;
    size_t len, cap;

} AstProgram;

typedef struct {

    Lexer *lexer;
    Token look;

} Parser;

Parser init_parser(Lexer *lexer);
AstProgram *parse_program(Parser *parser);
void free_program(AstProgram *program);
void free_token(Token *token);

#endif
