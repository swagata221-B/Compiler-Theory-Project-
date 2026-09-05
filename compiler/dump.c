#include "dump.h"

#include <stdio.h>

static const char *kind_name(NodeKind k) {
    switch (k) {
        case N_PROGRAM: return "Program";
        case N_IDENT: return "Ident";
        case N_VARDECL: return "VarDecl";
        case N_TYPE: return "Type";
        case N_ARRAY: return "ArrayType";
        case N_FUNCTION: return "Function";
        case N_PROCEDURE: return "Procedure";
        case N_PARAM: return "Params";
        case N_COMPOUND: return "BeginEnd";
        case N_ASSIGN: return "Assign";
        case N_CALL: return "Call";
        case N_IF: return "If";
        case N_WHILE: return "While";
        case N_FOR: return "For";
        case N_RETURN: return "Return";
        case N_EMPTY: return "Empty";
        case N_BINOP: return "Binary";
        case N_UNOP: return "Unary";
        case N_INT: return "Integer";
        case N_REAL: return "Real";
        case N_BOOL: return "Boolean";
        case N_STRING: return "String";
        case N_VAR: return "Var";
        default: return "Node";
    }
}

static void dump_one(const Node *n, int depth) {
    if (!n) return;
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
    if (n->line) printf("  [%d]", n->line);
    putchar('\n');

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
