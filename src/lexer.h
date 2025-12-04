#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include "errors.h"

typedef enum {

    TOK_EOF,
    TOK_NEWLINE,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
    TOK_COLON,
    TOK_ENTRY,
    TOK_OUT,
    TOK_LET,
    TOK_TOINT,
    TOK_TOSTR,
    TOK_IF,
    TOK_ELSE,
    TOK_STRING_T,
    TOK_INTEGER_T,
    TOK_FLOAT_T,
    TOK_BOOLEAN_T,
    TOK_VARIABLE,
    TOK_ASSIGN,
    TOK_ADD,
    TOK_SUBTRACT,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_FUNC,
    TOK_RETURN,
    TOK_VOID_T,
    TOK_STRING_LIT,
    TOK_INTEGER_LIT,
    TOK_FLOAT_LIT,
    TOK_BOOLEAN_LIT,
    TOK_UNKNOWN

} TokenType;

typedef struct {

    TokenType type;
    char *lexeme;
    int line;
    int column_start;
    int column_end;
    bool heap_allocated;

} Token;

typedef struct {

    const char *src;
    size_t pos;
    int line;
    int column;
    const char *file_path;

} Lexer;

Token next_token(Lexer *lexer);
const char *get_token_name(TokenType type);

#endif
