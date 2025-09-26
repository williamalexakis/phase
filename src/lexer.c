#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "errors.c"

typedef enum {

    TOK_EOF,          // EOF
    TOK_NEWLINE,      // \n
    TOK_LBRACE,       // {
    TOK_RBRACE,       // }
    TOK_LPAREN,       // (
    TOK_RPAREN,       // )
    TOK_COMMA,        // ,
    TOK_ENTRY,        // entry
    TOK_OUT,          // out()
    TOK_TOINT,        // toint()
    TOK_TOSTR,        // tostr()
    TOK_STRING_T,     // str type
    TOK_INTEGER_T,    // int type
    TOK_VARIABLE,     // var name
    TOK_ASSIGN,       // =
    TOK_STRING_LIT,   // str value
    TOK_INTEGER_LIT,  // int value
    TOK_UNKNOWN       // Any unrecognized lexeme

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

        // Skip whitespace (but not newlines)
        while (c == ' ' || c == '\t' || c == '\r') {

            advance_lexer(lexer);
            c = peek(lexer);

        }

        // Skip inline comments
        if (c == '-' && peek_2(lexer) == '-') {

            // Continue until newline
            while (c && c != '\n') c = advance_lexer(lexer);

            continue;

        }

        break;  // Alphanumeric char or newline is detected

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

    return is_ident_start(c) || (c >= '0' && c <='9');

}

static bool is_digit(char c) {

    return c >= '0' && c <= '9';

}

/*
 * Process an identifier or statement keyword and
 * produce the equivalent tokens
 */
static Token lex_ident_or_kw(Lexer *lexer) {

    int line = lexer->line;
    size_t start = lexer->pos;

    advance_lexer(lexer);  // We know first char is identifier start

    while (is_ident_part(peek(lexer))) advance_lexer(lexer);

    size_t end = lexer->pos;
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) {

        error_oom();

    }

    memcpy(lexeme, lexer->src + start, len);  // Copy str from memory
    lexeme[len] = '\0';

    if (strcmp(lexeme, "entry") == 0) return make_token(TOK_ENTRY, lexeme, line);
    if (strcmp(lexeme, "out") == 0) return make_token(TOK_OUT, lexeme, line);
    if (strcmp(lexeme, "toint") == 0) return make_token(TOK_TOINT, lexeme, line);
    if (strcmp(lexeme, "tostr") == 0) return make_token(TOK_TOSTR, lexeme, line);
    if (strcmp(lexeme, "int") == 0) return make_token(TOK_INTEGER_T, lexeme, line);
    if (strcmp(lexeme, "str") == 0) return make_token(TOK_STRING_T, lexeme, line);

    return make_token(TOK_VARIABLE, lexeme, line);  // Fallback is that it's a var name

}

/* Process a string literal and produce a token */
static Token lex_string(Lexer *lexer) {

    int line = lexer->line;

    advance_lexer(lexer);  // Skip opening "

    // Use a dynamic buffer to handle escape sequences
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

        // Handle escape sequences
        if (c == '\\') {
            advance_lexer(lexer);  // Skip backslash
            char next_c = peek(lexer);
            
            if (next_c == '\0') {
                error_open_str(lexer->line);
            }

            char escaped_char;
            switch (next_c) {
                case 'n': escaped_char = '\n'; break;
                case 't': escaped_char = '\t'; break;
                case 'r': escaped_char = '\r'; break;
                case '\\': escaped_char = '\\'; break;
                case '"': escaped_char = '"'; break;
                default: 
                    // If not a recognized escape sequence, keep both characters
                    escaped_char = '\\';
                    // Don't advance, so the next character gets processed normally
                    advance_lexer(lexer);
                    
                    // Expand buffer if needed
                    if (lexeme_len + 1 >= lexeme_cap) {
                        lexeme_cap *= 2;
                        lexeme = realloc(lexeme, lexeme_cap);
                        if (!lexeme) error_oom();
                    }
                    
                    lexeme[lexeme_len++] = escaped_char;
                    continue;  // Process the next character normally
            }
            
            advance_lexer(lexer);  // Skip the escaped character
            
            // Expand buffer if needed
            if (lexeme_len + 1 >= lexeme_cap) {
                lexeme_cap *= 2;
                lexeme = realloc(lexeme, lexeme_cap);
                if (!lexeme) error_oom();
            }
            
            lexeme[lexeme_len++] = escaped_char;
        } else {
            // Regular character
            advance_lexer(lexer);
            
            // Expand buffer if needed
            if (lexeme_len + 1 >= lexeme_cap) {
                lexeme_cap *= 2;
                lexeme = realloc(lexeme, lexeme_cap);
                if (!lexeme) error_oom();
            }
            
            lexeme[lexeme_len++] = c;
        }
    }

    lexeme[lexeme_len] = '\0';  // Null terminate

    advance_lexer(lexer);  // Skip closing "

    return make_token(TOK_STRING_LIT, lexeme, line);

}

static Token lex_int(Lexer *lexer) {

    int line = lexer->line;
    size_t start = lexer->pos;

    while (is_digit(peek(lexer))) advance_lexer(lexer);

    size_t end = lexer->pos;
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) {

        error_oom();

    }

    memcpy(lexeme, lexer->src + start, len);
    lexeme[len] = '\0';

    return make_token(TOK_INTEGER_LIT, lexeme, line);

}

/*
 * Handle the production of the next token from
 * the source, with the lexer
 */
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

    // Fallback if char is unrecognized
    advance_lexer(lexer);

    return make_token(TOK_UNKNOWN, NULL, lexer->line);

}

static bool is_heap_lexeme(Token token, const TokenType *list, const size_t len) {

    for (size_t i = 0; i < len; i++) {

        if (token.type == list[i]) return true;

    }

    return false;

}
