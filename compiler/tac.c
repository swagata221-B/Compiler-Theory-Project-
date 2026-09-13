#include "tac.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct Text {
    char *value;
    struct Text *next;
} Text;

typedef struct {
    Node *program;
    int temporary;
    int label;
    int failed;
    Text *texts;
    Text *lines;
    Text *tail;
} Tac;

static int same_name(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return 0;
    }
    return *a == *b;
}

static void fail(Tac *t, Node *n, const char *message) {
    fprintf(stderr, "TAC error at line %d: %s\n", n ? n->line : 0, message);
    t->failed = 1;
}

static char *format_text(Tac *t, const char *format, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int size = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    Text *entry = size >= 0 ? malloc(sizeof(*entry)) : NULL;
    char *value = entry ? malloc((size_t)size + 1) : NULL;
    if (!value) {
        free(entry);
        fail(t, NULL, "out of memory");
        return NULL;
    }
    vsnprintf(value, (size_t)size + 1, format, args);
    entry->value = value;
    entry->next = t->texts;
    t->texts = entry;
    return value;
}

static char *text(Tac *t, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char *value = format_text(t, format, args);
    va_end(args);
    return value;
}

/* Keep instructions in memory until lowering completes successfully. */
static void emit(Tac *t, const char *format, ...) {
    if (t->failed) return;
    va_list args;
    va_start(args, format);
    char *value = format_text(t, format, args);
    va_end(args);
    if (!value) return;
    Text *line = malloc(sizeof(*line));
    if (!line) { fail(t, NULL, "out of memory"); return; }
    line->value = value;
    line->next = NULL;
    if (t->tail) t->tail->next = line;
    else t->lines = line;
    t->tail = line;
}

static char *name(Tac *t, const char *source) {
    char *result = text(t, "%s", source);
    if (result) for (char *p = result; *p; ++p) *p = (char)tolower((unsigned char)*p);
    return result;
}

static int has_name(Node *n, const char *candidate) {
    for (; n; n = n->next) {
        if (n->kind == N_IDENT && same_name(n->text, candidate)) return 1;
        for (int i = 0; i < 4; ++i) if (has_name(n->child[i], candidate)) return 1;
    }
    return 0;
}

/* Skip user identifiers such as t1 so compiler temporaries never overwrite them. */
static char *new_temp(Tac *t) {
    char *result;
    do {
        result = text(t, "t%d", ++t->temporary);
    } while (result && has_name(t->program, result));
    return result;
}

static int is_io(const char *s) {
    return same_name(s, "read") || same_name(s, "readln") ||
           same_name(s, "write") || same_name(s, "writeln");
}

/* Validate the supported subset before printing any instructions. */
static void validate(Tac *t, Node *n) {
    for (; n && !t->failed; n = n->next) {
        if (n->kind == N_FUNCTION || n->kind == N_PROCEDURE || n->kind == N_RETURN) {
            fail(t, n, "subprograms and return are not supported by TAC generation yet");
        } else if (n->kind == N_ARRAY || (n->kind == N_VAR && n->child[1])) {
            fail(t, n, "arrays are not supported by TAC generation yet");
        } else if (n->kind == N_CALL) {
            const char *callee = n->child[0]->text;
            if (!is_io(callee)) {
                fail(t, n, "only read/readln/write/writeln calls are supported in TAC");
            } else if (same_name(callee, "read") || same_name(callee, "readln")) {
                for (Node *a = n->child[1]; a; a = a->next) {
                    if (a->kind != N_VAR) fail(t, a, "read requires a scalar variable");
                }
            }
        }
        for (int i = 0; i < 4 && !t->failed; ++i) validate(t, n->child[i]);
    }
}

static char *expression(Tac *t, Node *n) {
    if (!n || t->failed) return NULL;
    switch (n->kind) {
        case N_INT: return text(t, "%.0f", n->number);
        case N_REAL: return text(t, "%.17g", n->number);
        case N_BOOL: return text(t, "%s", n->number ? "true" : "false");
        case N_VAR: return name(t, n->child[0]->text);
        case N_STRING: {
            /* Pascal escapes a quote by doubling it. Keep each literal on one line. */
            size_t size = strlen(n->text);
            char *escaped = malloc(size * 2 + 3);
            if (!escaped) { fail(t, n, "out of memory"); return NULL; }
            char *p = escaped;
            *p++ = '\'';
            for (const char *s = n->text; *s; ++s) {
                if (*s == '\n' || *s == '\r' || *s == '\t' || *s == '\\') {
                    *p++ = '\\';
                    *p++ = *s == '\n' ? 'n' : *s == '\r' ? 'r' : *s == '\t' ? 't' : '\\';
                } else {
                    *p++ = *s;
                    if (*s == '\'') *p++ = '\'';
                }
            }
            *p++ = '\'';
            *p = '\0';
            char *result = text(t, "%s", escaped);
            free(escaped);
            return result;
        }
        case N_BINOP: {
            char *left = expression(t, n->child[0]);
            char *right = expression(t, n->child[1]);
            char *result = new_temp(t);
            if (!left || !right || !result) return NULL;
            emit(t, "%s = %s %s %s\n", result, left, n->text, right);
            return result;
        }
        case N_UNOP: {
            char *operand = expression(t, n->child[0]);
            char *result = new_temp(t);
            if (!operand || !result) return NULL;
            emit(t, "%s = %s %s\n", result, n->text, operand);
            return result;
        }
        default:
            fail(t, n, "unsupported expression in TAC");
            return NULL;
    }
}

static void statements(Tac *t, Node *n) {
    for (; n && !t->failed; n = n->next) {
        switch (n->kind) {
            case N_EMPTY: break;
            case N_COMPOUND: statements(t, n->child[0]); break;
            case N_ASSIGN: {
                char *value = expression(t, n->child[1]);
                char *target = name(t, n->child[0]->child[0]->text);
                if (value && target) emit(t, "%s = %s\n", target, value);
                break;
            }
            case N_IF: {
                int otherwise = ++t->label;
                int end = n->child[2] ? ++t->label : otherwise;
                char *condition = expression(t, n->child[0]);
                if (!condition) break;
                emit(t, "IF_FALSE %s GOTO L%d\n", condition, otherwise);
                statements(t, n->child[1]);
                if (n->child[2]) {
                    emit(t, "GOTO L%d\nL%d:\n", end, otherwise);
                    statements(t, n->child[2]);
                }
                emit(t, "L%d:\n", end);
                break;
            }
            case N_WHILE: {
                int start = ++t->label, end = ++t->label;
                emit(t, "L%d:\n", start);
                char *condition = expression(t, n->child[0]);
                if (!condition) break;
                emit(t, "IF_FALSE %s GOTO L%d\n", condition, end);
                statements(t, n->child[1]);
                emit(t, "GOTO L%d\nL%d:\n", start, end);
                break;
            }
            case N_FOR: {
                char *variable = name(t, n->child[0]->text);
                char *first = expression(t, n->child[1]);
                char *last = expression(t, n->child[2]);
                char *bound = new_temp(t);
                char *condition = new_temp(t);
                char *step = new_temp(t);
                if (!variable || !first || !last || !bound || !condition || !step) break;
                int start = ++t->label, end = ++t->label;
                int down = same_name(n->text, "downto");
                /* Snapshot the upper/lower bound once, before assigning the loop variable. */
                emit(t, "%s = %s\n%s = %s\nL%d:\n", bound, last, variable, first, start);
                emit(t, "%s = %s %s %s\nIF_FALSE %s GOTO L%d\n",
                        condition, variable, down ? ">=" : "<=", bound, condition, end);
                statements(t, n->child[3]);
                emit(t, "%s = %s %s 1\n%s = %s\nGOTO L%d\nL%d:\n",
                        step, variable, down ? "-" : "+", variable, step, start, end);
                break;
            }
            case N_CALL: {
                const char *callee = n->child[0]->text;
                int reading = same_name(callee, "read") || same_name(callee, "readln");
                for (Node *a = n->child[1]; a && !t->failed; a = a->next) {
                    char *value = expression(t, a);
                    if (value) emit(t, "%s %s\n", reading ? "READ" : "WRITE", value);
                }
                if (same_name(callee, "writeln")) emit(t, "WRITELN\n");
                break;
            }
            default: fail(t, n, "unsupported statement in TAC"); break;
        }
    }
}

int emit_tac(Node *program, FILE *out) {
    Tac t = {0};
    t.program = program;
    if (!program || program->kind != N_PROGRAM) return 1;
    validate(&t, program);
    if (t.failed) return 1;
    statements(&t, program->child[3]);
    if (!t.failed) {
        for (Text *line = t.lines; line; line = line->next) fputs(line->value, out);
        if (fflush(out) != 0 || ferror(out)) fail(&t, program, "cannot write TAC output");
    }
    while (t.lines) {
        Text *next = t.lines->next;
        free(t.lines);
        t.lines = next;
    }
    while (t.texts) {
        Text *next = t.texts->next;
        free(t.texts->value);
        free(t.texts);
        t.texts = next;
    }
    return t.failed ? 1 : 0;
}
