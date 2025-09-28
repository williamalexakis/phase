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
    TOK_TOINT,
    TOK_TOSTR,
    TOK_STRING_T,
    TOK_INTEGER_T,
    TOK_VARIABLE,
    TOK_ASSIGN,
    TOK_STRING_LIT,
    TOK_INTEGER_LIT,
    TOK_UNKNOWN

} TokenType;

typedef struct {

    TokenType type;
    char *lexeme;
    int line;

} Token;

typedef struct {

    const char *src;
    size_t pos;
    int line;

} Lexer;

/* Create a token with a type, lexeme, and line for error reporting */
static Token make_token(TokenType type, char *lexeme, int line) {

    return (Token) {.type = type, .lexeme = lexeme, .line = line};

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

    if (!lexeme) {

        error_oom();

    }

    memcpy(lexeme, lexer->src + start, len);
    lexeme[len] = '\0';

    if (strcmp(lexeme, "entry") == 0) return make_token(TOK_ENTRY, lexeme, line);
    if (strcmp(lexeme, "out") == 0) return make_token(TOK_OUT, lexeme, line);
    if (strcmp(lexeme, "toint") == 0) return make_token(TOK_TOINT, lexeme, line);
    if (strcmp(lexeme, "tostr") == 0) return make_token(TOK_TOSTR, lexeme, line);
    if (strcmp(lexeme, "int") == 0) return make_token(TOK_INTEGER_T, lexeme, line);
    if (strcmp(lexeme, "str") == 0) return make_token(TOK_STRING_T, lexeme, line);

    return make_token(TOK_VARIABLE, lexeme, line);

}

static Token lex_string(Lexer *lexer) {

    int line = lexer->line;

    advance_lexer(lexer);

    char *lexeme = malloc(64);
    size_t lexeme_len = 0;
    size_t lexeme_cap = 64;

    if (!lexeme) {
        error_oom();
    }

    for (;;) {

        char c = peek(lexer);

        if (c == '\0') {

            error_open_str(lexer->line);

        }

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

    return make_token(TOK_STRING_LIT, lexeme, line);

}

static Token lex_int(Lexer *lexer) {

    int line = lexer->line;
    size_t start = lexer->pos;

    while (is_digit(peek(lexer))) advance_lexer(lexer);

    size_t end = lexer->pos;
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) error_oom();

    memcpy(lexeme, lexer->src + start, len);
    lexeme[len] = '\0';

    return make_token(TOK_INTEGER_LIT, lexeme, line);

}

static Token next_token(Lexer *lexer) {

    ignore_ws_or_comment(lexer);

    char c = peek(lexer);

    if (c == '\0') return make_token(TOK_EOF, NULL, lexer->line);
    if (c == '\n') { advance_lexer(lexer); return make_token(TOK_NEWLINE, "\\n", lexer->line - 1); }
    if (c == '{') { advance_lexer(lexer); return make_token(TOK_LBRACE, "{", lexer->line); }
    if (c == '}') { advance_lexer(lexer); return make_token(TOK_RBRACE, "}", lexer->line); }
    if (c == '(') { advance_lexer(lexer); return make_token(TOK_LPAREN, "(", lexer->line); }
    if (c == ')') { advance_lexer(lexer); return make_token(TOK_RPAREN, ")", lexer->line); }
    if (c == ',') { advance_lexer(lexer); return make_token(TOK_COMMA, ",", lexer->line); }
    if (c == '=') { advance_lexer(lexer); return make_token(TOK_ASSIGN, "=", lexer->line); }
    if (c == '"') return lex_string(lexer);

    if (is_ident_start(c)) return lex_ident_or_kw(lexer);
    if (is_digit(c)) return lex_int(lexer);

    advance_lexer(lexer);

    return make_token(TOK_UNKNOWN, NULL, lexer->line);

}

/* Check if a token's lexeme needs to be freed */
static bool is_heap_lexeme(Token token, const TokenType *list, const size_t len) {

    for (size_t i = 0; i < len; i++) {

        if (token.type == list[i]) return true;

    }

    return false;

}
