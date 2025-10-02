#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "errors.c"

typedef enum {

    TOK_EOF,
    TOK_NEWLINE,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
    TOK_ENTRY,
    TOK_OUT,
    TOK_TOINT,        // WIP
    TOK_TOSTR,        // WIP
    TOK_STRING_T,
    TOK_INTEGER_T,
    TOK_FLOAT_T,
    TOK_BOOLEAN_T,
    TOK_VARIABLE,
    TOK_ASSIGN,
    TOK_ADD,          // WIP
    TOK_SUBTRACT,     // WIP
    TOK_MULTIPLY,     // WIP
    TOK_DIVIDE,       // WIP
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
    bool heap_allocated;

} Token;

typedef struct {

    const char *src;
    size_t pos;
    int line;

} Lexer;

/* Create a token with a type, lexeme, and line for error reporting */
static Token make_token(TokenType type, char *lexeme, int line, bool heap_allocated) {

    return (Token) {.type = type, .lexeme = lexeme, .line = line, .heap_allocated = heap_allocated};

}

static char peek(Lexer *lexer) { return lexer->src[lexer->pos]; }

static char peek_2(Lexer *lexer) {

    char c = peek(lexer);

    return c ? lexer->src[lexer->pos + 1] : '\0';

}

static char advance_lexer(Lexer *lexer) {

    char c = peek(lexer);

    if (c) {

        lexer->pos++;

        if (c == '\n') lexer->line++;

    }

    return c;

}

static void ignore_ws_or_comment(Lexer *lexer) {

    for (;;) {

        char c = peek(lexer);

        while (c == ' ' || c == '\t' || c == '\r') {

            advance_lexer(lexer);
            c = peek(lexer);

        }

        if (c == '-' && peek_2(lexer) == '-') {

            while (c && c != '\n') c = advance_lexer(lexer);

            continue;

        }

        break;

    }

}

static int is_ident_start(char c) {

    return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');

}

static int is_ident_part(char c) {

    return is_ident_start(c) || (c >= '0' && c <='9');

}

/* Check if a character is a digit */
static bool is_digit(char c) {

    return c >= '0' && c <= '9';

}

static Token lex_ident_or_kw(Lexer *lexer) {

    int line = lexer->line;
    size_t start = lexer->pos;

    advance_lexer(lexer);

    while (is_ident_part(peek(lexer))) advance_lexer(lexer);

    size_t end = lexer->pos;
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) error_oom();

    memcpy(lexeme, lexer->src + start, len);
    lexeme[len] = '\0';

    if (strcmp(lexeme, "entry") == 0) return make_token(TOK_ENTRY, lexeme, line, true);
    if (strcmp(lexeme, "out") == 0) return make_token(TOK_OUT, lexeme, line, true);
    if (strcmp(lexeme, "toint") == 0) return make_token(TOK_TOINT, lexeme, line, true);
    if (strcmp(lexeme, "tostr") == 0) return make_token(TOK_TOSTR, lexeme, line, true);
    if (strcmp(lexeme, "int") == 0) return make_token(TOK_INTEGER_T, lexeme, line, true);
    if (strcmp(lexeme, "str") == 0) return make_token(TOK_STRING_T, lexeme, line, true);
    if (strcmp(lexeme, "float") == 0) return make_token(TOK_FLOAT_T, lexeme, line, true);
    if (strcmp(lexeme, "bool") == 0) return make_token(TOK_BOOLEAN_T, lexeme, line, true);
    if (strcmp(lexeme, "true") == 0) return make_token(TOK_BOOLEAN_LIT, lexeme, line, true);
    if (strcmp(lexeme, "false") == 0) return make_token(TOK_BOOLEAN_LIT, lexeme, line, true);

    return make_token(TOK_VARIABLE, lexeme, line, true);

}

static Token lex_string(Lexer *lexer) {

    int line = lexer->line;

    advance_lexer(lexer);

    char *lexeme = malloc(64);
    size_t lexeme_len = 0;
    size_t lexeme_cap = 64;

    if (!lexeme) error_oom();

    for (;;) {

        char c = peek(lexer);

        if (c == '\0') error_open_str(lexer->line);
        if (c == '"') break;

        if (c == '\\') {

            advance_lexer(lexer);

            char next_c = peek(lexer);

            if (next_c == '\0') error_open_str(lexer->line);

            char escaped_char;

            switch (next_c) {

                case 'n': escaped_char = '\n'; break;
                case 't': escaped_char = '\t'; break;
                case 'r': escaped_char = '\r'; break;
                case '\\': escaped_char = '\\'; break;
                case '"': escaped_char = '"'; break;

                default:

                    escaped_char = '\\';
                    advance_lexer(lexer);

                    if (lexeme_len + 1 >= lexeme_cap) {

                        lexeme_cap *= 2;
                        lexeme = realloc(lexeme, lexeme_cap);

                        if (!lexeme) error_oom();

                    }

                    lexeme[lexeme_len++] = escaped_char;

                    continue;
            }

            advance_lexer(lexer);

            if (lexeme_len + 1 >= lexeme_cap) {

                lexeme_cap *= 2;
                lexeme = realloc(lexeme, lexeme_cap);

                if (!lexeme) error_oom();

            }

            lexeme[lexeme_len++] = escaped_char;

        } else {

            advance_lexer(lexer);

            if (lexeme_len + 1 >= lexeme_cap) {

                lexeme_cap *= 2;
                lexeme = realloc(lexeme, lexeme_cap);

                if (!lexeme) error_oom();

            }

            lexeme[lexeme_len++] = c;

        }

    }

    lexeme[lexeme_len] = '\0';

    advance_lexer(lexer);

    return make_token(TOK_STRING_LIT, lexeme, line, true);

}

static Token lex_number(Lexer *lexer) {

    int line = lexer->line;
    size_t start = lexer->pos;
    bool is_float = false;

    while (is_digit(peek(lexer))) advance_lexer(lexer);

    if (peek(lexer) == '.' && is_digit(peek_2(lexer))) {

        is_float = true;
        advance_lexer(lexer);

        while (is_digit(peek(lexer))) advance_lexer(lexer);

    }

    size_t end = lexer->pos;
    size_t len = end - start;
    char *lexeme = malloc(len + 1);

    if (!lexeme) error_oom();

    memcpy(lexeme, lexer->src + start, len);

    lexeme[len] = '\0';
    TokenType type = is_float ? TOK_FLOAT_LIT : TOK_INTEGER_LIT;

    return make_token(type, lexeme, line, true);

}

static Token next_token(Lexer *lexer) {

    ignore_ws_or_comment(lexer);

    char c = peek(lexer);

    switch (c) {

        case '\0': return make_token(TOK_EOF, NULL, lexer->line, false);
        case '\n': advance_lexer(lexer); return make_token(TOK_NEWLINE, "\\n", lexer->line - 1, false);
        case '{': advance_lexer(lexer); return make_token(TOK_LBRACE, "{", lexer->line, false);
        case '}': advance_lexer(lexer); return make_token(TOK_RBRACE, "}", lexer->line, false);
        case '(': advance_lexer(lexer); return make_token(TOK_LPAREN, "(", lexer->line, false);
        case ')': advance_lexer(lexer); return make_token(TOK_RPAREN, ")", lexer->line, false);
        case ',': advance_lexer(lexer); return make_token(TOK_COMMA, ",", lexer->line, false);
        case '=': advance_lexer(lexer); return make_token(TOK_ASSIGN, "=", lexer->line, false);
        case '+': advance_lexer(lexer); return make_token(TOK_ADD, "+", lexer->line, false);
        case '-': advance_lexer(lexer); return make_token(TOK_SUBTRACT, "-", lexer->line, false);
        case '*': advance_lexer(lexer); return make_token(TOK_MULTIPLY, "*", lexer->line, false);
        case '/': advance_lexer(lexer); return make_token(TOK_DIVIDE, "/", lexer->line, false);
        case '"': return lex_string(lexer);

    }

    if (is_ident_start(c)) return lex_ident_or_kw(lexer);
    if (is_digit(c)) return lex_number(lexer);

    advance_lexer(lexer);

    return make_token(TOK_UNKNOWN, NULL, lexer->line, false);

}

/* Get token type name for displaying in token mode */
static const char *get_token_name(TokenType type) {

    static const char *token_names[] = {

        [TOK_EOF] = "EOF",
        [TOK_NEWLINE] = "NEWLINE",
        [TOK_LBRACE] = "LEFT BRACE",
        [TOK_RBRACE] = "RIGHT BRACE",
        [TOK_LPAREN] = "LEFT PAREN",
        [TOK_RPAREN] = "RIGHT PAREN",
        [TOK_COMMA] = "COMMA",
        [TOK_ENTRY] = "ENTRY",
        [TOK_OUT] = "OUT",
        [TOK_TOINT] = "TOINT",
        [TOK_TOSTR] = "TOSTR",
        [TOK_STRING_T] = "STRING TYPE",
        [TOK_INTEGER_T] = "INTEGER TYPE",
        [TOK_FLOAT_T] = "FLOAT TYPE",
        [TOK_BOOLEAN_T] = "BOOLEAN TYPE",
        [TOK_VARIABLE] = "VARIABLE",
        [TOK_ASSIGN] = "ASSIGN",
        [TOK_ADD] = "ADD",
        [TOK_SUBTRACT] = "SUBTRACT",
        [TOK_MULTIPLY] = "MULTIPLY",
        [TOK_DIVIDE] = "DIVIDE",
        [TOK_STRING_LIT] = "STRING LITERAL",
        [TOK_INTEGER_LIT] = "INTEGER LITERAL",
        [TOK_FLOAT_LIT] = "FLOAT LITERAL",
        [TOK_BOOLEAN_LIT] = "BOOLEAN LITERAL",
        [TOK_UNKNOWN] = "UNKNOWN"

    };

    if (type >= 0 && type < sizeof(token_names) / sizeof(token_names[0])) {

        return token_names[type];

    }

    return "INVALID";

}
