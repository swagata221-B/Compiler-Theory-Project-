#include "dump.h"

#include <stdio.h>

static const char *kind_name(NodeKind k) {
    switch (k) {
        case N_PROGRAM: return "Program";
        case N_IDENT: return "Identifier";
        case N_VARDECL: return "Variable declaration";
        case N_TYPE: return "Type";
        case N_ARRAY: return "ArrayType";
        case N_FUNCTION: return "Function";
        case N_PROCEDURE: return "Procedure";
        case N_PARAM: return "Params";
        case N_COMPOUND: return "Block";
        case N_ASSIGN: return "Assignment";
        case N_CALL: return "Call";
        case N_IF: return "If";
        case N_WHILE: return "While";
        case N_FOR: return "For";
        case N_RETURN: return "Return";
        case N_EMPTY: return "Empty";
        case N_BINOP: return "Binary operation";
        case N_UNOP: return "Unary operation";
        case N_INT: return "Integer";
        case N_REAL: return "Real";
        case N_BOOL: return "Boolean";
        case N_STRING: return "String";
        case N_VAR: return "Variable reference";
        default: return "Node";
    }
}

static void dump_one(const Node *n, int depth) {
    if (!n || n->kind == N_EMPTY) return;
    for (int i = 0; i < depth; i++) fputs("  ", stdout);
    fputs(kind_name(n->kind), stdout);
    if (n->kind == N_INT)
        printf(" %ld", (long)n->number);
    else if (n->kind == N_REAL)
        printf(" %g", n->number);
    else if (n->kind == N_BOOL)
        printf(" %s", n->number ? "true" : "false");
    else if (n->text)
        printf(" %s", n->text);
    if (n->line) printf("  [line %d]", n->line);
    if (n->kind == N_PROGRAM || n->kind == N_VARDECL || n->kind == N_COMPOUND ||
        n->kind == N_ASSIGN || n->kind == N_CALL || n->kind == N_IF ||
        n->kind == N_WHILE || n->kind == N_FOR || n->kind == N_RETURN) {
        const char *source = source_line(n->line);
        if (source) {
            while (*source == ' ' || *source == '\t') ++source;
            printf(" | %s", source);
        }
    }
    putchar('\n');

    if (n->kind == N_ASSIGN || n->kind == N_BINOP || n->kind == N_IF) {
        const char *roles[3];
        int count = 2;
        if (n->kind == N_ASSIGN) { roles[0] = "Target"; roles[1] = "Value"; }
        else if (n->kind == N_BINOP) { roles[0] = "Left operand"; roles[1] = "Right operand"; }
        else { roles[0] = "Condition"; roles[1] = "Then"; roles[2] = "Else"; count = 3; }
        for (int j = 0; j < count; ++j) {
            if (!n->child[j]) continue;
            for (int i = 0; i < depth + 1; ++i) fputs("  ", stdout);
            puts(roles[j]);
            dump_one(n->child[j], depth + 2);
        }
        return;
    }

    if (n->kind == N_FUNCTION || n->kind == N_PROCEDURE) {
        dump_one(n->child[0], depth + 1);
        if (n->child[1]) {
            for (int i = 0; i < depth + 1; i++) fputs("  ", stdout);
            puts("Parameters");
            for (Node *p = n->child[1]; p; p = p->next) dump_one(p, depth + 2);
        }
        if (n->child[2]) dump_one(n->child[2], depth + 1);
        if (n->child[3]) {
            if (n->child[3]->child[1]) {
                for (int i = 0; i < depth + 1; i++) fputs("  ", stdout);
                puts("Locals");
                for (Node *p = n->child[3]->child[1]; p; p = p->next)
                    dump_one(p, depth + 2);
            }
            dump_one(n->child[3], depth + 1);
        }
        return;
    }

    if (n->kind == N_COMPOUND) {
        for (Node *p = n->child[0]; p; p = p->next) dump_one(p, depth + 1);
        return;
    }

    if (n->kind == N_VARDECL || n->kind == N_PARAM) {
        for (Node *p = n->child[0]; p; p = p->next) dump_one(p, depth + 1);
        dump_one(n->child[1], depth + 1);
        return;
    }

    if (n->kind == N_PROGRAM) {
        dump_one(n->child[0], depth + 1);
        if (n->child[1]) {
            for (int i = 0; i < depth + 1; i++) fputs("  ", stdout);
            puts("Declarations");
            for (Node *p = n->child[1]; p; p = p->next) dump_one(p, depth + 2);
        }
        if (n->child[2]) {
            for (int i = 0; i < depth + 1; i++) fputs("  ", stdout);
            puts("Subprograms");
            for (Node *p = n->child[2]; p; p = p->next) dump_one(p, depth + 2);
        }
        dump_one(n->child[3], depth + 1);
        return;
    }

    if (n->kind == N_CALL) {
        dump_one(n->child[0], depth + 1);
        for (Node *p = n->child[1]; p; p = p->next) dump_one(p, depth + 1);
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (!n->child[i]) continue;
        /* child lists that use next (args already handled) */
        dump_one(n->child[i], depth + 1);
        if (n->kind == N_ARRAY && i < 2) continue;
    }
}

void dump_tree(const Node *n) {
    dump_one(n, 0);
}
