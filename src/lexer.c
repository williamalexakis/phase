#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "errors.c"

typedef enum {

    T_EOF,      // EOF
    T_NEWLINE,  // \n
    T_LBRACE,   // {
    T_RBRACE,   // }
    T_ENTRY,    // entry
    T_OUT,      // out
    T_STRING,   // "string"
    T_UNKNOWN   // Any unrecognized lexeme

} TokenType;

typedef struct {

    TokenType type;
    char *lexeme;
    int line;  // Line number for errors

} Token;

typedef struct {

    const char *src;  // Source str
    size_t pos;       // Cursor position
    int line;         // Current line number for errors

} Lexer;

/*
 * Create a token struct with a type, lexeme,
 * and line for error reporting
 */
static Token make_token(TokenType type, char *lexeme, int line) {

    return (Token) {.type = type, .lexeme = lexeme, .line = line};

}

/* Check the current char the lexer cursor is on */
static char peek(Lexer *lexer) { return lexer->src[lexer->pos]; }

/*
 * Check a char proceeding the current cursor pos,
 * intended for 2-char symbols such as comments '--'
 */
static char peek_2(Lexer *lexer) {

    char c = peek(lexer);
    return c ? lexer->src[lexer->pos + 1] : '\0';

}

/* Move the lexer cursor to the next char in the src */
static char advance_lexer(Lexer *lexer) {

    char c = peek(lexer);

    if (c) {

        lexer->pos++;

        if (c == '\n') lexer->line++;

    }

    return c;

}

/* Skip whitespace, escape chars, and comments */
static void ignore_ws_or_comment(Lexer *lexer) {

    for (;;) {

        char c = peek(lexer);

        // Skip whitespace
        while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {

            advance_lexer(lexer);
            c = peek(lexer);

        }

        // Skip inline comments
        if (c == '-' && peek_2(lexer) == '-') {

            // Continue until newline
            while (c && c != '\n') c = advance_lexer(lexer);

            continue;

        }

        break;  // Alphanumeric char is detected

    }

}

/*
 * Check if the current char is the start of
 * an identifier or statement keyword
 */
static int is_ident_start(char c) {

    return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');

}

/*
 * Check if the current char is within
 * an identifier or statement keyword
 */
static int is_ident_part(char c) {

    return is_ident_start(c) || (c>='0'&&c<='9');

}

/*
 * Process an identifier or statement keyword and
 * produce the equivalent tokens
 */
static Token lex_ident_or_kw(Lexer *lexer) {

    int line = lexer->line;
    size_t start = lexer->pos;

    advance_lexer(lexer);  // We know first char is identifier start

    while( is_ident_part(peek(lexer))) advance_lexer(lexer);

    size_t end = lexer->pos;
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) {

        fprintf(stderr, "[phasec] ERROR: OUT OF MEMORY\n");
        free(lexeme);

        exit(1);

    }

    memcpy(lexeme, lexer->src + start, len);  // Copy str from memory
    lexeme[len] = '\0';  // Add a null terminator so str ops don't explode

    if (strcmp(lexeme, "entry") == 0) return make_token(T_ENTRY, lexeme, line);
    if (strcmp(lexeme, "out") == 0) return make_token(T_OUT, lexeme, line);

    return make_token(T_UNKNOWN, lexeme, line);  // Unrecognized lexeme

}

/* Process a string literal and produce a token */
static Token lex_string(Lexer *lexer) {

    int line = lexer->line;

    advance_lexer(lexer);  // Skip opening "

    size_t start = lexer->pos;

    for (;;) {

        char c = peek(lexer);

        if (c == '\0') {

            fprintf(stderr, "[phasec] ERROR: UNTERMINATED STRING (LINE %d)\n", line);
            exit(1);

        }

        if (c == '"') break;

        advance_lexer(lexer);

    }

    size_t end = lexer->pos;  // At closing "
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) {

        fprintf(stderr, "[phasec] ERROR: OUT OF MEMORY\n");
        free(lexeme);

        exit(1);

    }

    memcpy(lexeme, lexer->src + start, len);  // Copy the str into memory
    lexeme[len] = '\0';  // Null term for str ops

    advance_lexer(lexer);  // Skip closing "

    return make_token(T_STRING, lexeme, line);

}

/*
 * Handle the production of the next token from
 * the source, with the lexer
 */
static Token next_token(Lexer *lexer) {

    ignore_ws_or_comment(lexer);

    char c = peek(lexer);

    if (c == '\0') return make_token(T_EOF, NULL, lexer->line);
    if (c == '{') { advance_lexer(lexer); return make_token(T_LBRACE, "{", lexer->line); }
    if (c == '}') { advance_lexer(lexer); return make_token(T_RBRACE, "}", lexer->line); }
    if (c == '"') return lex_string(lexer);
    if (is_ident_start(c)) return lex_ident_or_kw(lexer);

    // Fallback if char is unrecognized
    advance_lexer(lexer);

    return make_token(T_UNKNOWN, NULL, lexer->line);

}

static bool is_heap_lexeme(Token token, const TokenType *list, const size_t len) {

    for (size_t i = 0; i < len; i++) {

        if (token.type == list[i]) return true;

    }

    return false;

}
