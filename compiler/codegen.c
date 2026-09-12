#include "codegen.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static FILE *out;
static int errors;
static Node *decls;

static void fail(Node *n, const char *message) {
    fprintf(stderr, "codegen error at line %d: %s\n", n ? n->line : 0, message);
    errors++;
}

static int same(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return 0;
    return !*a && !*b;
}

static Node *lookup(const char *name) {
    for (Node *d = decls; d; d = d->next)
        for (Node *i = d->child[0]; i; i = i->next)
            if (same(i->text, name)) return d->child[1];
    return NULL;
}

static void name(const char *s) {
    fputs("mp_", out);
    for (; s && *s; s++) fputc(tolower((unsigned char)*s), out);
}

static void quoted(const char *s) {
    fputc('"', out);
    for (; s && *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '\\' || c == '"') { fputc('\\', out); fputc(c, out); }
        else if (c == '\n') fputs("\\n", out);
        else if (c == '\r') fputs("\\r", out);
        else if (c == '\t') fputs("\\t", out);
        else if (c < 32) fprintf(out, "\\%03o", c);
        else fputc(c, out);
    }
    fputc('"', out);
}

static void expr(Node *n) {
    if (!n) { fputs("0", out); return; }
    switch (n->kind) {
        case N_INT: case N_REAL: case N_BOOL: fprintf(out, "%.17g", n->number); break;
        case N_STRING: quoted(n->text); break;
        case N_VAR: {
            const char *id = n->child[0] ? n->child[0]->text : NULL;
            Node *type = lookup(id);
            if (!type) { fail(n, "undeclared variable"); fputs("0", out); break; }
            if (!!n->child[1] != (type->kind == N_ARRAY)) {
                fail(n, "array index mismatch"); fputs("0", out); break;
            }
            name(id);
            if (n->child[1]) {
                fprintf(out, "[(int)("); expr(n->child[1]);
                fprintf(out, ") - %ld]", (long)type->child[0]->number);
            }
            break;
        }
        case N_UNOP:
            fputc('(', out);
            fputs(same(n->text, "not") ? "!" : n->text, out);
            expr(n->child[0]); fputc(')', out); break;
        case N_BINOP: {
            const char *op = n->text;
            if (same(op, "div") || same(op, "mod")) {
                fputs("((long)(", out); expr(n->child[0]);
                fputs(")", out); fputs(same(op, "mod") ? " % " : " / ", out);
                fputs("(long)(", out); expr(n->child[1]); fputs("))", out);
            } else {
                fputc('(', out); expr(n->child[0]);
                fprintf(out, " %s ", same(op, "and") ? "&&" : same(op, "or") ? "||" : same(op, "=") ? "==" : same(op, "<>") ? "!=" : op);
                expr(n->child[1]); fputc(')', out);
            }
            break;
        }
        default: fail(n, "unsupported expression in native backend"); fputs("0", out); break;
    }
}

static void stmt(Node *n) {
    for (; n; n = n->next) {
        switch (n->kind) {
            case N_EMPTY: break;
            case N_COMPOUND: stmt(n->child[0]); break;
            case N_ASSIGN:
                expr(n->child[0]); fputs(" = ", out); expr(n->child[1]); fputs(";\n", out); break;
            case N_CALL: {
                const char *id = n->child[0] ? n->child[0]->text : "";
                if (!same(id, "write") && !same(id, "writeln")) {
                    fail(n, "only write/writeln calls are supported in native backend"); break;
                }
                for (Node *a = n->child[1]; a; a = a->next) {
                    if (a->kind == N_STRING) { fputs("fputs(", out); expr(a); fputs(", stdout);\n", out); }
                    else { fputs("mp_print(", out); expr(a); fputs(");\n", out); }
                }
                if (same(id, "writeln")) fputs("putchar('\\n');\n", out);
                break;
            }
            case N_IF:
                fputs("if (", out); expr(n->child[0]); fputs(") {\n", out);
                stmt(n->child[1]); fputs("}\n", out);
                if (n->child[2]) { fputs("else {\n", out); stmt(n->child[2]); fputs("}\n", out); }
                break;
            case N_WHILE:
                fputs("while (", out); expr(n->child[0]); fputs(") {\n", out);
                stmt(n->child[1]); fputs("}\n", out); break;
            default: fail(n, "unsupported statement in native backend"); break;
        }
    }
}

int generate_c(Node *program, const char *path) {
    if (!program || !path) return 1;
    out = fopen(path, "w");
    if (!out) { perror(path); return 1; }
    errors = 0;
    decls = program->child[1];
    if (program->child[2]) fail(program->child[2], "functions and procedures are not supported in native backend");
    fputs("#include <stdio.h>\nstatic void mp_print(double x) { if (x == (long)x) printf(\"%ld \", (long)x); else printf(\"%g \", x); }\nint main(void) {\n", out);
    for (Node *d = decls; d; d = d->next) {
        Node *t = d->child[1];
        if (!t || (t->kind != N_TYPE && t->kind != N_ARRAY)) { fail(d, "unsupported declaration"); continue; }
        long size = t->kind == N_ARRAY ? (long)(t->child[1]->number - t->child[0]->number + 1) : 1;
        if (size < 1 || size > 1000000) { fail(d, "invalid array size"); continue; }
        for (Node *i = d->child[0]; i; i = i->next) {
            fprintf(out, "double "); name(i->text);
            if (t->kind == N_ARRAY) fprintf(out, "[%ld] = {0};\n", size);
            else fputs(" = 0;\n", out);
        }
    }
    stmt(program->child[3]);
    fputs("return 0;\n}\n", out);
    if (fclose(out) != 0) errors++;
    if (errors) { remove(path); return 1; }
    return 0;
}
