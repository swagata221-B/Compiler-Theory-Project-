#include "ast.h"

#include <stdlib.h>
#include <string.h>

Node *ast_root = NULL;
int error_count = 0;

char *copy_cstr(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

Node *node(NodeKind kind, int line) {
    Node *n = calloc(1, sizeof(Node));
    n->kind = kind;
    n->line = line;
    return n;
}

Node *ident(const char *name, int line) {
    Node *n = node(N_IDENT, line);
    n->text = copy_cstr(name);
    return n;
}

Node *integer_lit(long value, int line) {
    Node *n = node(N_INT, line);
    n->number = (double)value;
    return n;
}

Node *real_lit(double value, const char *raw, int line) {
    Node *n = node(N_REAL, line);
    n->number = value;
    n->text = copy_cstr(raw);
    return n;
}

Node *string_lit(const char *value, int line) {
    Node *n = node(N_STRING, line);
    n->text = copy_cstr(value);
    return n;
}

Node *bool_lit(int value, int line) {
    Node *n = node(N_BOOL, line);
    n->number = value ? 1 : 0;
    return n;
}

Node *binop(const char *op, Node *left, Node *right, int line) {
    Node *n = node(N_BINOP, line);
    n->text = copy_cstr(op);
    n->child[0] = left;
    n->child[1] = right;
    return n;
}

Node *unop(const char *op, Node *arg, int line) {
    Node *n = node(N_UNOP, line);
    n->text = copy_cstr(op);
    n->child[0] = arg;
    return n;
}

Node *append(Node *list, Node *item) {
    if (!list) return item;
    Node *p = list;
    while (p->next) p = p->next;
    p->next = item;
    return list;
}

void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->next);
    for (int i = 0; i < 4; i++) free_tree(n->child[i]);
    free(n->text);
    free(n);
}
