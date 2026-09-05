#ifndef AST_H
#define AST_H

typedef enum {
    N_PROGRAM,
    N_IDENT,
    N_VARDECL,
    N_TYPE,
    N_ARRAY,
    N_FUNCTION,
    N_PROCEDURE,
    N_PARAM,
    N_COMPOUND,
    N_ASSIGN,
    N_CALL,
    N_IF,
    N_WHILE,
    N_FOR,
    N_RETURN,
    N_EMPTY,
    N_BINOP,
    N_UNOP,
    N_INT,
    N_REAL,
    N_BOOL,
    N_STRING,
    N_VAR
} NodeKind;

typedef struct Node {
    NodeKind kind;
    int line;
    int col;
    char *text;
    double number;
    struct Node *child[4];
    struct Node *next;
} Node;

Node *node(NodeKind kind, int line);
Node *ident(const char *name, int line);
Node *integer_lit(long value, int line);
Node *real_lit(double value, const char *raw, int line);
Node *string_lit(const char *value, int line);
Node *bool_lit(int value, int line);
Node *binop(const char *op, Node *left, Node *right, int line);
Node *unop(const char *op, Node *arg, int line);
Node *append(Node *list, Node *item);
char *copy_cstr(const char *s);
void free_tree(Node *n);

extern Node *ast_root;
extern int error_count;

#endif
