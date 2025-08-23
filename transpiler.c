#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Lexer */

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
Token make_token(TokenType type, char *lexeme, int line) {

    return (Token) {.type = type, .lexeme = lexeme, .line = line};

}

/* Check the current char the lexer cursor is on */
char peek(Lexer *lexer) { return lexer->src[lexer->pos]; }

/*
 * Check a char proceeding the current cursor pos,
 * intended for 2-char symbols such as comments '--'
 */
char peek_2(Lexer *lexer) {

    char c = peek(lexer);
    return c ? lexer->src[lexer->pos + 1] : '\0';

}

/* Move the lexer cursor to the next char in the src */
char advance(Lexer *lexer) {

    char c = peek(lexer);

    if (c) {

        lexer->pos++;

        if (c == '\n') lexer->line++;

    }

    return c;

}

/* Skip whitespace, escape chars, and comments */
void ignore_ws_or_comment(Lexer *lexer) {

    for (;;) {

        char c = peek(lexer);

        // Skip whitespace
        while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {

            advance(lexer);
            c = peek(lexer);

        }

        // Skip inline comments
        if (c == '-' && peek_2(lexer) == '-') {

            // Continue until newline
            while (c && c != '\n') c = advance(lexer);

            continue;

        }

        break;  // Alphanumeric char is detected

    }

}

/*
 * Check if the current char is the start of
 * an identifier or statement keyword
 */
int is_ident_start(char c) {

    return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');

}

/*
 * Check if the current char is within
 * an identifier or statement keyword
 */
int is_ident_part(char c) {

    return is_ident_start(c) || (c>='0'&&c<='9');

}

/*
 * Process an identifier or statement keyword and
 * produce the equivalent tokens
 */
Token lex_ident_or_kw(Lexer *lexer) {

    int line = lexer->line;
    size_t start = lexer->pos;

    advance(lexer);  // We know first char is identifier start

    while( is_ident_part(peek(lexer))) advance(lexer);

    size_t end = lexer->pos;
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) {

        fprintf(stderr, "[zirc] ERROR: OUT OF MEMORY\n");
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
Token lex_string(Lexer *lexer) {

    int line = lexer->line;

    advance(lexer);  // Skip opening "

    size_t start = lexer->pos;

    for (;;) {

        char c = peek(lexer);

        if (c == '\0') {

            fprintf(stderr, "[zirc] ERROR: UNTERMINATED STRING (LINE %d)\n", line);
            exit(1);

        }

        if (c == '\n') {

            fprintf(stderr, "[zirc] ERROR: NEWLINE IN STRING LITERAL (LINE %d)\n", line);
            exit(1);

        }

        if (c == '"') break;

        advance(lexer);

    }

    size_t end = lexer->pos;  // At closing "
    size_t len = end - start;

    char *lexeme = malloc(len + 1);

    if (!lexeme) {

        fprintf(stderr, "[zirc] ERROR: OUT OF MEMORY\n");
        free(lexeme);

        exit(1);

    }

    memcpy(lexeme, lexer->src + start, len);  // Copy the str into memory
    lexeme[len] = '\0';  // Null term for str ops

    advance(lexer);  // Skip closing "

    return make_token(T_STRING, lexeme, line);

}

/*
 * Handle the production of the next token from
 * the source, with the lexer
 */
Token next_token(Lexer *lexer) {

    ignore_ws_or_comment(lexer);

    char c = peek(lexer);

    if (c == '\0') return make_token(T_EOF, NULL, lexer->line);
    if (c == '{') { advance(lexer); return make_token(T_LBRACE, "{", lexer->line); }
    if (c == '}') { advance(lexer); return make_token(T_RBRACE, "}", lexer->line); }
    if (c == '"') return lex_string(lexer);
    if (is_ident_start(c)) return lex_ident_or_kw(lexer);

    // Fallback if char is unrecognized
    advance(lexer);

    return make_token(T_UNKNOWN, NULL, lexer->line);

}

/* Main */
int main(int argc, char **argv) {

    // Check if not enough args are provided
    if (argc < 2) {

        fprintf(stderr, "[zirc] ERROR: INSUFFICIENT ARGUMENTS PROVIDED\n");
        exit(1);

    }

    // Check if help flag is used
    if (strcmp(argv[1], "--help") == 0) {

        fprintf(stderr, "[zirc] USAGE: zirc <input.zir> <output.c>\n");
        exit(0);

    }

    FILE *input_file = fopen(argv[1], "r");  // Open the input file

    // Check if file is not found
    if (input_file == NULL) {

        fprintf(stderr, "[zirc] ERROR: INPUT FILE '%s' NOT FOUND\n", argv[1]);
        exit(1);

    }

    // Figure out the file size by moving the file ptr to the end
    fseek(input_file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(input_file);
    rewind(input_file); // Return file ptr to start

    // Allocate buffer large enough for the file + null terminator
    char *file_content = malloc(file_size + 1);

    if (!file_content) {

        fprintf(stderr, "[zirc] ERROR: OUT OF MEMORY\n");
        free(file_content);
        fclose(input_file);

        exit(1);

    }

    // Read the whole file into the buffer
    fread(file_content, sizeof(char), file_size, input_file);
    file_content[file_size] = '\0';  // Append null terminator for str ops

    fclose(input_file);

    // Initialize lexer
    Lexer lexer = { .src = file_content, .pos = 0, .line = 1 };

    // Display the tokenized form of the source
    // file, including the src lexemes and line
    // numbers
    for (;;) {

        Token token = next_token(&lexer);

        const char *token_names[] = {

            "EOF",
            "NEWLINE",
            "LBRACE",
            "RBRACE",
            "ENTRY",
            "OUT",
            "STRING",
            "UNKNOWN"

        };

        printf("%s", token_names[token.type]);              // Display token type
        if (token.lexeme) printf(" \"%s\"", token.lexeme);  // Display the lexeme
        printf(" [LINE %d]\n", token.line);                 // Display line num

        // Free the lexeme only if it was allocated memory,
        // in this case only if the token is an ident, kw,
        // or string
        if (token.lexeme && (token.type == T_OUT || token.type == T_ENTRY || token.type == T_STRING)) {

            free(token.lexeme);

        }

        if (token.type == T_EOF) break;

    }

    free(file_content);

    return 0;

}
