#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

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
char advance_lexer(Lexer *lexer) {

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

    advance_lexer(lexer);  // We know first char is identifier start

    while( is_ident_part(peek(lexer))) advance_lexer(lexer);

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

    advance_lexer(lexer);  // Skip opening "

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

        advance_lexer(lexer);

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

    advance_lexer(lexer);  // Skip closing "

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
    if (c == '{') { advance_lexer(lexer); return make_token(T_LBRACE, "{", lexer->line); }
    if (c == '}') { advance_lexer(lexer); return make_token(T_RBRACE, "}", lexer->line); }
    if (c == '"') return lex_string(lexer);
    if (is_ident_start(c)) return lex_ident_or_kw(lexer);

    // Fallback if char is unrecognized
    advance_lexer(lexer);

    return make_token(T_UNKNOWN, NULL, lexer->line);

}

bool is_heap_lexeme(Token token, const TokenType *list, const size_t len) {

    for (size_t i = 0; i < len; i++) {

        if (token.type == &list[i]) return true;

    }

    return false;

}

/* Parser */

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

    DECL_ENTRY

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

void vector_push(void ***items, size_t *len, size_t *cap, void *elt) {

    if (*len + 1 > *cap) {

        *cap = *cap ? *cap * 2 : 8;
        *items = realloc(*items, *cap * sizeof(void*));

        if (!items) {

            fprintf(stderr, "[zirc] ERROR: OUT OF MEMORY\n");
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

Parser init_parser(Lexer *lexer) {

    Parser parser = { .lexer = lexer, .look = next_token(lexer) };

    return parser;

}

void free_token(Token *token) {

    TokenType heap_tokens[3] = {T_OUT, T_ENTRY, T_STRING};
    size_t ht_len = sizeof(heap_tokens) / sizeof(heap_tokens[0]);
    bool needs_freeing = is_heap_lexeme(*token, heap_tokens, ht_len);

    if (token->lexeme && needs_freeing) free(token->lexeme);
    token->lexeme = NULL;

}

void advance_parser(Parser *parser) {

    if (parser->look.type != T_EOF) free_token(&parser->look);

    parser->look = next_token(parser->lexer);

}

bool match(Parser *parser, TokenType t_type) {

    if (parser->look.type == t_type) {

        advance_parser(parser);

        return true;

    }

    return false;

}

void expect(Parser *parser, TokenType t_type,const char *message) {

    if (!match(parser, t_type)) {

        fprintf(stderr, "[zirc] ERROR (LINE %d): EXPECTED %s\n", parser->look.line, message);
        exit(1);

    }

}

AstExpression *parse_expression(Parser *parser) {

    if (parser->look.type == T_STRING) {

        AstExpression *expression = calloc(1, sizeof(*expression));

        expression->tag = E_STRING;
        expression->line = parser->look.line;
        expression->str_lit.value = strdup(parser->look.lexeme ? parser->look.lexeme : "");

        advance_parser(parser);

        return expression;

    }

    fprintf(stderr, "[zirc] ERROR (LINE %d): EXPECTED EXPRESSION\n", parser->look.line);
    exit(1);

}

AstStatement *parse_statement(Parser *parser) {

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

    fprintf(stderr, "[zirc] ERROR (LINE %d): EXPECTED STATEMENT\n", parser->look.line);
    exit(1);

}

AstBlock *parse_block(Parser *parser) {

    expect(parser, T_LBRACE, "'{'");
    AstBlock *block = calloc(1, sizeof(*block));

    while (parser->look.type != T_RBRACE) {

        AstStatement *statement = parse_statement(parser);
        vector_push((void***)&block->statements, &block->len, &block->cap, statement);

    }

    expect(parser, T_RBRACE, "'}'");

    return block;

}

AstDeclare *parse_entry_decl(Parser *parser) {

    int line = parser->look.line;
    expect(parser, T_ENTRY, "'entry'");

    AstBlock *block = parse_block(parser);
    AstDeclare *declare = calloc(1, sizeof(*declare));

    declare->tag = DECL_ENTRY;
    declare->line = line;
    declare->entry.block = block;

    return declare;

}

AstProgram *parse_program(Parser *parser) {

    AstProgram *program = calloc(1, sizeof(*program));

    while (parser->look.type != T_EOF) {

        AstDeclare *declare = NULL;

        if (parser->look.type == T_ENTRY) {

            declare = parse_entry_decl(parser);

        } else {

            fprintf(stderr, "[zirc] ERROR (LINE %d): UNEXPECTED TOPLEVEL TOKEN\n", parser->look.line);
            exit(1);

        }

        vector_push((void***)&program->decls, &program->len, &program->cap, declare);

    }

    return program;

}

void free_expression(AstExpression *expression) {

    if (!expression) return;

    switch (expression->tag) {

        case E_STRING: free(expression->str_lit.value); break;

    }

    free(expression);

}

void free_statement(AstStatement *statement) {

    if (!statement) return;

    switch (statement->tag) {

        case S_OUT: free_expression(statement->out.expression); break;

    }

    free(statement);

}

void free_block(AstBlock *block) {

    if (!block) return;

    for (size_t i = 0; i < block->len; i++) free_statement(block->statements[i]);

    free(block->statements);
    free(block);

}

void free_declaration(AstDeclare *declare) {

    if (!declare) return;

    switch (declare->tag) {

        case DECL_ENTRY: free_block(declare->entry.block); break;

    }

    free(declare);

}

void free_program(AstProgram *program) {

    if (!program) return;

    for (size_t i = 0; i < program->len; i++);

    free(program->decls);
    free(program);

}

/* Codegen */

typedef struct {

    FILE *output;
    int indent;

} Emitter;

void emit(Emitter *emitter, const char *str) { fputs(str, emitter->output); }
void emit_char(Emitter *emitter, char c) { fputc(c, emitter->output); }
void indent_increase(Emitter *emitter) { emitter->indent += 4; }
void indent_decrease(Emitter *emitter) { emitter->indent -= 4; }

void emit_newline(Emitter *emitter) {

    fputc('\n', emitter->output);
    for (int i = 0; i < emitter->indent; i++) fputc(' ', emitter->output);

}

char *escape_c_str(const char *str) {

    size_t cap = strlen(str) * 2 + 16;
    char *output = malloc(cap);

    if (!output) {

        fprintf(stderr, "[zirc] ERROR: OUT OF MEMORY\n");
        free(output);

        exit(1);

    };

    size_t n = 0;

    for (size_t i = 0; str[i]; i++) {

        unsigned char c = (unsigned char)str[i];

        if (n + 4 >= cap) {

            cap *= 2;
            output = realloc(output, cap);

        }

        if (c == '\"') {

            output[n++] = '\\';
            output[n++] = '\"';

        } else if (c == '\\') {

            output[n++] = '\\';
            output[n++] = '\\';

        } else if (c == '\n') {

            output[n++] = '\\';
            output[n++] = 'n';

        } else if (c == '\t') {

            output[n++] = '\\';
            output[n++] = 't';

        } else if (c < 32) {

            n += (size_t)sprintf(output + n, "\\x%02x", c);

        } else {

            output[n++] = (char)c;

        }

    }

    output[n] = '\0';

    return output;

}

void emit_expression(Emitter *emitter, AstExpression *expression) {

    switch (expression->tag) {

        case E_STRING: {

            char *escape = escape_c_str(expression->str_lit.value);

            emit(emitter, "\"");
            emit(emitter, escape);
            emit(emitter, "\\n\"");

            free(escape);

        } break;

    }

}

void emit_statement(Emitter *emitter, AstStatement *statement) {

    switch (statement->tag) {

        case S_OUT: {

            emit(emitter, "printf(");
            emit_expression(emitter, statement->out.expression);
            emit(emitter, ");");
            emit_newline(emitter);

        } break;

    }

}

void emit_block(Emitter *emitter, AstBlock *block) {

    emit(emitter, "{");
    indent_increase(emitter);
    emit_newline(emitter);

    for (size_t i = 0; i < block->len; i++) emit_statement(emitter, block->statements[i]);

    indent_decrease(emitter);
    emit_newline(emitter);
    emit(emitter, "}");
    emit_newline(emitter);

}

void emit_declaration(Emitter *emitter, AstDeclare *declare, bool *entry_exists) {

    switch (declare->tag) {

        case DECL_ENTRY: {

            if (*entry_exists) {

                fprintf(stderr, "[zirc] ERROR: CANNOT HAVE MULTIPLE PROGRAM ENTRYPOINTS\n");
                exit(1);

            }

            emit(emitter, "int main(void) ");
            emit(emitter, "{");
            indent_increase(emitter);
            emit_newline(emitter);

            AstBlock *block = declare->entry.block;

            for (size_t i = 0; i < block->len; i++) emit_statement(emitter, block->statements[i]);

            emit_newline(emitter); emit_newline(emitter);
            emit(emitter, "return 0;");
            indent_decrease(emitter);
            emit_newline(emitter);
            emit(emitter, "}");
            emit_newline(emitter);

            *entry_exists = true;

        } break;

    }

}

void emit_program(Emitter *emitter, AstProgram *program) {

    emit(emitter, "#include <stdio.h>\n");
    emit_newline(emitter);
    bool entry_exists = false;

    for (size_t i = 0; i < program->len; i++) {

        emit_declaration(emitter, program->decls[i], &entry_exists);

    }

    if (!entry_exists) {

        fprintf(stderr, "[zirc] ERROR: NO PROGRAM ENTRYPOINT FOUND\n");
        exit(1);

    }

}

// /* AST printer */

// void indent(int n) { for (int i = 0; i < n; i++) putchar(' '); }

// void print_expression(AstExpression *expression, int ind) {

//     switch (expression->tag) {

//         case E_STRING:

//             indent(ind);
//             printf("EXPRESSION (STRING) \"%s\"\n", expression->str_lit.value);
//             break;

//     }

// }

// void print_statement(AstStatement *statement, int ind) {

//     switch (statement->tag) {

//         case S_OUT:

//             indent(ind);
//             printf("STATEMENT (OUT)");
//             print_expression(statement->out.expression, ind + 4);
//             break;

//     }

// }

// void print_block(AstBlock *block, int ind) {

//     indent(ind);
//     printf("BLOCK\n");

//     for (size_t i = 0; i < block->len; i++) print_statement(block->statements[i], ind + 4);

// }

// void print_declaration(AstDeclare *declare, int ind) {

//     switch (declare->tag) {

//         case DECL_ENTRY:

//             indent(ind);
//             printf("DECLARATION (ENTRY)\n");
//             print_block(declare->entry.block, ind + 4);
//             break;

//     }

// }

// void print_program(AstProgram *program) {

//     printf("PROGRAM\n");

//     for (size_t i = 0; i < program->len; i++) print_declaration(program->decls[i], 4);

// }

/* Main */
int main(int argc, char **argv) {

    // Check if not enough args are provided
    if (argc < 2) {

        fprintf(stderr, "[zirc] ERROR: INSUFFICIENT ARGUMENTS PROVIDED\n");
        exit(1);

    }

    // Check if help flag is used
    if (strcmp(argv[1], "--help") == 0) {

        fprintf(stderr, "[zirc] USAGE: zirc <input.zrc> <output.c>\n");
        exit(0);

    }

    FILE *input_file = fopen(argv[1], "r");  // Open the input file

    // Check if file is not found
    if (!input_file) {

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

    FILE *output_file = fopen(argv[2], "w");

    if (!output_file) {

        fprintf(stderr, "[zirc] ERROR: OUTPUT FILE '%s' NOT FOUND\n", argv[2]);
        exit(1);

    }

    // Initialize lexer
    Lexer lexer = { .src = file_content, .pos = 0, .line = 1 };
    Parser parser = init_parser(&lexer);
    AstProgram *program = parse_program(&parser);
    Emitter emitter = { .output = output_file, .indent = 0 };

    emit_program(&emitter, program);

    fclose(output_file);

    free_program(program);
    free_token(&parser.look);
    free(file_content);

    // Display the tokenized form of the source
    // file, including the src lexemes and line
    // numbers
    // for (;;) {

    //     Token token = next_token(&lexer);

    //     const char *token_names[] = {

    //         "EOF",
    //         "NEWLINE",
    //         "LBRACE",
    //         "RBRACE",
    //         "ENTRY",
    //         "OUT",
    //         "STRING",
    //         "UNKNOWN"

    //     };

    //     printf("%s", token_names[token.type]);              // Display token type
    //     if (token.lexeme) printf(" \"%s\"", token.lexeme);  // Display the lexeme
    //     printf(" [LINE %d]\n", token.line);                 // Display line num

    //     // Use a linear search to determine if
    //     // the token has an allocated lexeme
    //     const TokenType heap_tokens[3] = {T_OUT, T_ENTRY, T_STRING};
    //     const size_t ht_len = sizeof(heap_tokens) / sizeof(heap_tokens[0]);
    //     const bool needs_freeing = is_heap_lexeme(token, heap_tokens, ht_len);

    //     // Free the lexeme only if it was allocated memory,
    //     // in this case only if the token is an ident, kw,
    //     // or string
    //     if (token.lexeme && needs_freeing) {

    //         free(token.lexeme);

    //     }

    //     if (token.type == T_EOF) break;

    // }

    return 0;

}

/*
 * TODO:
 * - Make error manager with separate .c/.h files
 * - Improve error display system
 * - Make funcs out of token/AST display and add
 *   them as --flagged options
 * - Add --flag to just print output to stdout
 * - Add --flag for emitting .o file or running
 *   output directly instead, plus --flag to specify
 *   the C compiler path to do either of these
 * - Add some more documentation to the parser and
 *   codegen
 * - Begin the file modularization process (we're
 *   cooked)
 */
