#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include "colours.h"
#include "errors.h"

typedef struct {

    ErrorType code;
    const char *message_fmt;
    const char *help_fmt;
    const char *error_colour;
    const char *help_colour;
    char *(*suggest)(const char *line_text, ErrorLocation loc, va_list args);

} ErrorInfo;

static char *suggest_insert_expected(const char *line_text, ErrorLocation loc, va_list args);
static char *suggest_remove_span(const char *line_text, ErrorLocation loc, va_list args);
static char *suggest_type_mismatch_fix(const char *line_text, ErrorLocation loc, va_list args);
static char *suggest_close_string(const char *line_text, ErrorLocation loc, va_list args);

static const ErrorInfo ERROR_TABLE[] = {

    { ERR_OOM, "Out of memory.", "Reduce memory usage or increase its capacity.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_OPEN_STR, "Unterminated string.", "Use a closing '\"' to end a string.", FG_RED_BOLD, FG_BLUE_BOLD, suggest_close_string },
    { ERR_EXPECT_SYMBOL, "Expected %s.", "Add %s here.", FG_RED_BOLD, FG_BLUE_BOLD, suggest_insert_expected },
    { ERR_EXPECT_EXPRESSION, "Expected expression.", "Provide an expression at this position.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_EXPECT_STATEMENT, "Expected statement or declaration.", "Provide a statement or declaration here.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_INVALID_TOK, "Unexpected token at global scope.", "Only 'entry' blocks or 'let' declarations are valid at global scope; remove or rewrite this token.", FG_RED_BOLD, FG_BLUE_BOLD, suggest_remove_span },
    { ERR_MANY_ENTRY, "Duplicate entry block.", "Only one 'entry' block is allowed.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_NO_ENTRY, "Missing entry block.", "Add an 'entry' block to define the program entrypoint.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_TYPE_MISMATCH, "Type mismatch.", "Variable '%s' expects %s but got %s.", FG_RED_BOLD, FG_BLUE_BOLD, suggest_type_mismatch_fix },
    { ERR_INVALID_OPCODE, "Unknown opcode '%d'.", "Unavailable (Internal Error).", FG_RED_BOLD, FG_PURPLE_BOLD, NULL },
    { ERR_INVALID_VAR_INDEX, "Invalid variable index.", "Index out of range; maximum is %zu variables.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_INVALID_CONST_INDEX, "Invalid constant index.", "Index out of range; maximum is %zu constants.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_VM_POS_OOB, "VM pointer out of bounds.", "Unavailable (Internal Error).", FG_RED_BOLD, FG_PURPLE_BOLD, NULL },
    { ERR_UNDEFINED_VAR, "Variable '%s' is undefined.", "Variables must be declared before use.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_UNEXPECTED_IDENT, "Unexpected identifier '%s'.", "Use 'let %s: <type>' to declare or '%s = <expr>' to assign.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_WRONG_VAR_INIT, "Variable initialization mismatch.", "Declared %zu variables but found %zu initializers.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_NO_ARGS, "Missing input file.", "Pass an input file path (<input_file.phase>).", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_INVALID_ARG, "Unknown argument '%s'.", "See all available arguments with 'phase --help'.", FG_RED_BOLD, FG_BLUE_BOLD, NULL },
    { ERR_NO_INPUT, "Input file '%s' not found.", "Use a valid input path (e.g. /path/to/file.phase).", FG_RED_BOLD, FG_BLUE_BOLD, NULL }

};

static const char *g_error_file = NULL;

noreturn void exit_phase(unsigned int code) {
    if (code == 0) {
        fprintf(stderr, "\nProcess successfully exited with code %d.\n", code);
    } else if (code == 1) {
        fprintf(stderr, "\nProcess exited with code %d.\n", code);
    } else if (code == 2) {
        exit(0);
    }
    exit(code);
}

void error_set_source(const char *file) {

    g_error_file = file;

}

static const ErrorInfo *find_error_info(ErrorType code) {

    size_t count = sizeof(ERROR_TABLE) / sizeof(ERROR_TABLE[0]);

    for (size_t i = 0; i < count; i++) {

        if (ERROR_TABLE[i].code == code) return &ERROR_TABLE[i];

    }

    return NULL;

}

static ErrorLocation normalize_location(ErrorLocation loc) {

    if (!loc.file) loc.file = g_error_file;
    if (loc.col_end < loc.col_start && loc.col_end != 0) loc.col_end = loc.col_start;

    return loc;

}

static char *load_line_from_file(const char *path, int target_line) {

    if (!path || target_line <= 0) return NULL;

    FILE *file = fopen(path, "r");

    if (!file) return NULL;

    size_t cap = 256;
    size_t len = 0;
    char *buffer = malloc(cap);
    int current_line = 1;
    int ch;

    if (!buffer) {

        fclose(file);
        return NULL;

    }

    while ((ch = fgetc(file)) != EOF) {

        if (current_line == target_line) {

            if (ch == '\n') {

                buffer[len] = '\0';
                fclose(file);
                return buffer;

            }

            if (len + 1 >= cap) {

                cap *= 2;
                char *new_buf = realloc(buffer, cap);

                if (!new_buf) {

                    free(buffer);
                    fclose(file);
                    return NULL;

                }

                buffer = new_buf;

            }

            buffer[len++] = (char)ch;

        }

        if (ch == '\n') {

            if (current_line == target_line) break;
            current_line++;

            if (current_line > target_line) break;

        }

    }

    fclose(file);

    if (current_line == target_line) {

        buffer[len] = '\0';
        return buffer;

    }

    free(buffer);
    return NULL;

}

static void print_source_snippet(const char *line_text, ErrorLocation loc, const char *bar_side) {

    if (!line_text) return;

    int line_no = loc.line > 0 ? loc.line : 0;
    int col_start = loc.col_start > 0 ? loc.col_start : 1;
    int col_end = loc.col_end >= col_start ? loc.col_end : col_start;

    char num_buf[32];
    int width = snprintf(num_buf, sizeof(num_buf), "%d", line_no);

    fprintf(stderr, "%s%s%s\n", FG_RED_BOLD, bar_side, RESET);
    fprintf(stderr, "%s%s %*d | %s%s\n", FG_RED_BOLD, bar_side, width, line_no, RESET, line_text);
    fprintf(stderr, "%s%s %*s | %s", FG_RED_BOLD, bar_side, width, "", RESET);

    for (int i = 1; i < col_start; i++) fputc(' ', stderr);
    for (int i = col_start; i <= col_end; i++) fputc('^', stderr);

    fputc('\n', stderr);
    fprintf(stderr, "%s%s%s\n", FG_RED_BOLD, bar_side, RESET);

}

static char *trim_expected_token(const char *expected) {

    if (!expected) return NULL;

    while (*expected == ' ' || *expected == '\t') expected++;

    size_t len = strlen(expected);

    while (len > 0 && (expected[len - 1] == ' ' || expected[len - 1] == '\t')) len--;

    if (len == 0) return NULL;

    if (len >= 2 && ((expected[0] == '\'' && expected[len - 1] == '\'') || (expected[0] == '"' && expected[len - 1] == '"'))) {

        expected++;
        len -= 2;

    }

    char *clean = malloc(len + 1);

    if (!clean) return NULL;

    memcpy(clean, expected, len);
    clean[len] = '\0';

    return clean;

}

static char *suggest_insert_expected(const char *line_text, ErrorLocation loc, va_list args) {

    if (!line_text) return NULL;

    const char *expected = va_arg(args, const char *);
    char *token = trim_expected_token(expected);

    if (!token) return NULL;

    for (size_t i = 0; token[i]; i++) {

        if (isalnum((unsigned char)token[i])) {

            free(token);
            return NULL;

        }

    }

    size_t line_len = strlen(line_text);
    size_t insert_len = strlen(token);
    size_t pos = (loc.col_start > 0) ? (size_t)(loc.col_start - 1) : 0;

    if (pos > line_len) pos = line_len;

    char *out = malloc(line_len + insert_len + 1);

    if (!out) {

        free(token);
        return NULL;

    }

    memcpy(out, line_text, pos);
    memcpy(out + pos, token, insert_len);
    memcpy(out + pos + insert_len, line_text + pos, line_len - pos);
    out[line_len + insert_len] = '\0';

    free(token);

    return out;

}

static char *suggest_remove_span(const char *line_text, ErrorLocation loc, va_list args) {

    (void)args;

    if (!line_text) return NULL;

    size_t line_len = strlen(line_text);
    size_t start = (loc.col_start > 0) ? (size_t)(loc.col_start - 1) : 0;
    size_t end = (loc.col_end > 0) ? (size_t)(loc.col_end - 1) : start;

    if (start > line_len) start = line_len;
    if (end >= line_len) end = line_len ? line_len - 1 : 0;
    if (end < start) end = start;

    size_t remove_len = (end >= start) ? (end - start + 1) : 0;
    if (remove_len > line_len) remove_len = line_len;

    size_t new_len = line_len - remove_len;
    char *out = malloc(new_len + 1);

    if (!out) return NULL;

    memcpy(out, line_text, start);
    memcpy(out + start, line_text + end + 1, line_len - end - 1);
    out[new_len] = '\0';

    return out;

}

static const char *placeholder_for_expected(const char *expected) {

    if (!expected) return "/* fix type */";

    if (strcmp(expected, "int") == 0) return "0";
    if (strcmp(expected, "float") == 0) return "0.0";
    if (strcmp(expected, "str") == 0 || strcmp(expected, "string") == 0) return "\"\"";
    if (strcmp(expected, "bool") == 0 || strcmp(expected, "boolean") == 0) return "false";

    return "/* fix type */";

}

static char *suggest_close_string(const char *line_text, ErrorLocation loc, va_list args) {

    (void)loc;
    (void)args;

    if (!line_text) return NULL;

    size_t len = strlen(line_text);
    size_t start = (loc.col_start > 0) ? (size_t)(loc.col_start - 1) : 0;
    size_t insert_pos = len;

    for (size_t i = start; i < len; i++) {

        if (line_text[i] == ')' || line_text[i] == ',' || line_text[i] == ';') {

            insert_pos = i;
            break;

        }

    }

    char *out = malloc(len + 2);

    if (!out) return NULL;

    memcpy(out, line_text, insert_pos);
    out[insert_pos] = '"';
    memcpy(out + insert_pos + 1, line_text + insert_pos, len - insert_pos);
    out[len + 1] = '\0';

    return out;

}

static char *suggest_type_mismatch_fix(const char *line_text, ErrorLocation loc, va_list args) {

    (void)loc;

    const char *var_name = va_arg(args, const char *);
    const char *expected = va_arg(args, const char *);
    const char *actual = va_arg(args, const char *);

    if (!line_text || !var_name || !expected || !actual) return NULL;

    size_t indent_len = 0;

    while (line_text[indent_len] == ' ' || line_text[indent_len] == '\t') indent_len++;

    const char *type_start = line_text + indent_len;
    const char *type_keywords[] = { "int", "float", "str", "bool" };
    size_t matched_len = 0;

    for (size_t i = 0; i < sizeof(type_keywords) / sizeof(type_keywords[0]); i++) {

        size_t len = strlen(type_keywords[i]);

        if (strncmp(type_start, type_keywords[i], len) == 0) {

            matched_len = len;
            break;

        }

    }

    if (matched_len > 0) {

        size_t prefix_len = indent_len;
        size_t suffix_len = strlen(type_start + matched_len);
        size_t actual_len = strlen(actual);

        char *out = malloc(prefix_len + actual_len + suffix_len + 1);

        if (!out) return NULL;

        memcpy(out, line_text, prefix_len);
        memcpy(out + prefix_len, actual, actual_len);
        memcpy(out + prefix_len + actual_len, type_start + matched_len, suffix_len);
        out[prefix_len + actual_len + suffix_len] = '\0';

        return out;

    }

    const char *placeholder = placeholder_for_expected(expected);

    size_t line_len = strlen(line_text);
    size_t var_len = strlen(var_name);
    size_t placeholder_len = strlen(placeholder);
    size_t out_len = indent_len + var_len + 3 + placeholder_len;

    char *out = malloc(out_len + 1);

    if (!out) return NULL;

    memcpy(out, line_text, indent_len);
    memcpy(out + indent_len, var_name, var_len);
    memcpy(out + indent_len + var_len, " = ", 3);
    memcpy(out + indent_len + var_len + 3, placeholder, placeholder_len);

    out[out_len] = '\0';

    (void)line_len;

    return out;

}

/* Check if we can use unicode glyphs in errors */
bool unicode_available(void) {

    const char *term = getenv("TERM");

    if (term) {

        if (strcmp(term, "dumb") == 0) return false;
        if (strstr(term, "vt100") != NULL) return false;
        if (strstr(term, "ansi") != NULL) return false;

    }

    const char *loc = setlocale(LC_CTYPE, NULL);
    const char *env_ct = getenv("LC_CTYPE");
    const char *env_lang = getenv("LANG");
    const char *candidates[] = { loc, env_ct, env_lang };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {

        const char *val = candidates[i];

        if (!val) continue;
        if (strstr(val, "UTF-8")) return true;
        if (strstr(val, "utf-8")) return true;
        if (strstr(val, "utf8")) return true;
        if (strstr(val, "UTF8")) return true;

    }

    return true;

}

static noreturn void error_emit(ErrorLocation loc, ErrorType code, ...) {
    
    bool unicode = unicode_available();
    const char *bar_main = unicode ? "┏" : ">";
    const char *bar_sub = unicode ? "┣" : ">";
    const char *bar_side = unicode ? "┃" : "|";
    const ErrorInfo *info = find_error_info(code);

    if (!info) {

        fprintf(stderr, "%s%s Fatal Error [%d]:%s Unknown error.\n", FG_RED_BOLD, bar_main, code, RESET);
        fprintf(stderr, "%s%s Help:%s Unavailable (INTERNAL ERROR).\n", FG_PURPLE_BOLD, bar_sub, RESET);
        exit_phase(1);

    }

    loc = normalize_location(loc);

    const char *file = loc.file ? loc.file : "<unknown>";
    int line = loc.line > 0 ? loc.line : 0;
    int col_start = loc.col_start > 0 ? loc.col_start : 1;
    int col_end = loc.col_end > 0 ? loc.col_end : col_start;
    bool has_location = line > 0;

    char *line_text = NULL;
    if (has_location) line_text = load_line_from_file(loc.file, loc.line);

    va_list args;
    va_start(args, code);

    va_list args_msg;
    va_copy(args_msg, args);

    fprintf(stderr, "%s%s Fatal Error [%d]:%s ", info->error_colour, bar_main, info->code, RESET);
    vfprintf(stderr, info->message_fmt, args_msg);
    fprintf(stderr, "\n");

    va_end(args_msg);

    if (has_location) {

        fprintf(stderr, "%s%s -->%s %s:%d:%d-%d%s\n", FG_RED_BOLD, bar_side, RESET, file, line, col_start, col_end, RESET);
        print_source_snippet(line_text, loc, bar_side);

    }

    va_list args_help;
    va_copy(args_help, args);

    fprintf(stderr, "%s%s Help:%s ", info->help_colour, bar_sub, RESET);
    vfprintf(stderr, info->help_fmt, args_help);
    fprintf(stderr, "\n");

    va_end(args_help);

    char *suggested_line = NULL;

    if (has_location && info->suggest && line_text) {

        va_list args_suggest;
        va_copy(args_suggest, args);
        suggested_line = info->suggest(line_text, loc, args_suggest);
        va_end(args_suggest);

        if (suggested_line) {

            fprintf(stderr, "%s%s Suggestion:%s\n", info->help_colour, bar_side, RESET);
            fprintf(stderr, "%s%s%s %s- %s%s\n", FG_BLUE_BOLD, bar_side, RESET, FG_RED, line_text, RESET);
            fprintf(stderr, "%s%s%s %s+ %s%s\n", FG_BLUE_BOLD, bar_side, RESET, FG_GREEN, suggested_line, RESET);

        }

    }

    va_end(args);
    if (suggested_line) free(suggested_line);
    if (line_text) free(line_text);

    exit_phase(1);
}

// Internal errors
void error_oom(void) { error_emit((ErrorLocation){0}, ERR_OOM); }
void error_open_str(ErrorLocation loc) { error_emit(loc, ERR_OPEN_STR); }
void error_expect_symbol(ErrorLocation loc, const char *expected) { error_emit(loc, ERR_EXPECT_SYMBOL, expected); }
void error_expect_expression(ErrorLocation loc) { error_emit(loc, ERR_EXPECT_EXPRESSION); }
void error_expect_statement(ErrorLocation loc) { error_emit(loc, ERR_EXPECT_STATEMENT); }
void error_invalid_token(ErrorLocation loc) { error_emit(loc, ERR_INVALID_TOK); }
void error_multiple_entry(ErrorLocation loc) { error_emit(loc, ERR_MANY_ENTRY); }
void error_no_entry(void) { error_emit((ErrorLocation){0}, ERR_NO_ENTRY); }
void error_type_mismatch(ErrorLocation loc, const char *var_name, const char *expected_type, const char *actual_type) {
    error_emit(loc, ERR_TYPE_MISMATCH, var_name, expected_type, actual_type);
}

void error_invalid_opcode(ErrorLocation loc, int op) { error_emit(loc, ERR_INVALID_OPCODE, op); }
void error_invalid_var_index(ErrorLocation loc, size_t var_count) { error_emit(loc, ERR_INVALID_VAR_INDEX, var_count); }
void error_invalid_const_index(ErrorLocation loc, size_t const_count) { error_emit(loc, ERR_INVALID_CONST_INDEX, const_count); }
void error_vm_oob(ErrorLocation loc) { error_emit(loc, ERR_VM_POS_OOB); }
void error_wrong_var_init(ErrorLocation loc, size_t var_count, size_t init_count) { error_emit(loc, ERR_WRONG_VAR_INIT, var_count, init_count); }
void error_undefined_var(ErrorLocation loc, const char *name) { error_emit(loc, ERR_UNDEFINED_VAR, name); }
void error_unexpected_ident(ErrorLocation loc, const char *name) { error_emit(loc, ERR_UNEXPECTED_IDENT, name, name); }

// CLI errors
void error_no_args(void) { error_emit((ErrorLocation){0}, ERR_NO_ARGS); }
void error_invalid_arg(const char *arg) { error_emit((ErrorLocation){0}, ERR_INVALID_ARG, arg); }
void error_ifnf(const char *name) { error_emit((ErrorLocation){ .file = name }, ERR_NO_INPUT, name); }
