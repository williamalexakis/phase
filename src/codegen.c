#include "parser.c"

typedef struct {

    FILE *output;
    int indent;

} Emitter;

static void emit(Emitter *emitter, const char *str) { fputs(str, emitter->output); }  /* Emit a string */
static void emit_char(Emitter *emitter, char c) { fputc(c, emitter->output); }  /* Emit a char */
static void indent_increase(Emitter *emitter) { emitter->indent += 4; }  /* Increment the indent */
static void indent_decrease(Emitter *emitter) { emitter->indent -= 4; }  /* Decrement the indent */

static void emit_newline(Emitter *emitter) {

    fputc('\n', emitter->output);
    for (int i = 0; i < emitter->indent; i++) fputc(' ', emitter->output);

}

static char *escape_c_str(const char *str) {

    size_t cap = strlen(str) * 2 + 16;
    char *output = malloc(cap);

    if (!output) {

        fprintf(stderr, "[phasec] ERROR: OUT OF MEMORY\n");
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

static void emit_expression(Emitter *emitter, AstExpression *expression) {

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

static void emit_statement(Emitter *emitter, AstStatement *statement) {

    switch (statement->tag) {

        case S_OUT: {

            emit(emitter, "printf(");
            emit_expression(emitter, statement->out.expression);
            emit(emitter, ");");
            emit_newline(emitter);

        } break;

    }

}

static void emit_block(Emitter *emitter, AstBlock *block) {

    emit(emitter, "{");
    indent_increase(emitter);
    emit_newline(emitter);

    for (size_t i = 0; i < block->len; i++) emit_statement(emitter, block->statements[i]);

    indent_decrease(emitter);
    emit_newline(emitter);
    emit(emitter, "}");
    emit_newline(emitter);

}

static void emit_declaration(Emitter *emitter, AstDeclare *declare, bool *entry_exists) {

    switch (declare->tag) {

        case D_ENTRY: {

            if (*entry_exists) {

                fprintf(stderr, "[phasec] ERROR: CANNOT HAVE MULTIPLE PROGRAM ENTRYPOINTS\n");
                exit(1);

            }

            emit(emitter, "int main(void) ");
            emit(emitter, "{");
            indent_increase(emitter);
            emit_newline(emitter);

            AstBlock *block = declare->entry.block;

            for (size_t i = 0; i < block->len; i++) emit_statement(emitter, block->statements[i]);

            emit_newline(emitter);
            emit(emitter, "return 0;");
            indent_decrease(emitter);
            emit_newline(emitter);
            emit(emitter, "}");
            emit_newline(emitter);

            *entry_exists = true;

        } break;

    }

}

static void emit_program(Emitter *emitter, AstProgram *program) {

    emit(emitter, "#include <stdio.h>\n");
    emit_newline(emitter);
    bool entry_exists = false;

    for (size_t i = 0; i < program->len; i++) {

        emit_declaration(emitter, program->decls[i], &entry_exists);

    }

    if (!entry_exists) {

        fprintf(stderr, "[phasec] ERROR: NO PROGRAM ENTRYPOINT FOUND\n");
        exit(1);

    }

}
