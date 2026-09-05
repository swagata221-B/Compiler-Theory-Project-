#include "interp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Var {
    char *name;
    int is_array;
    long lo;
    long hi;
    double *data;
    struct Var *next;
} Var;

typedef struct Frame {
    Var *vars;
    const char *fun_name;
    double ret;
    int has_ret;
    struct Frame *prev;
} Frame;

typedef struct {
    int is_str;
    double num;
    const char *str;
} Val;

static Frame *top;
static Node *unit;
static int run_errors;
static int leave;

static int ieq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = *a++, cb = *b++;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
    }
    return *a == *b;
}

static void runtime_error(int line, const char *msg) {
    fprintf(stderr, "runtime error at line %d: %s\n", line, msg);
    run_errors++;
}

static Var *find_var(const char *name) {
    for (Frame *f = top; f; f = f->prev) {
        for (Var *v = f->vars; v; v = v->next) {
            if (ieq(v->name, name)) return v;
        }
    }
    return NULL;
}

static Var *make_var(const char *name, int is_array, long lo, long hi) {
    Var *v = calloc(1, sizeof(Var));
    v->name = copy_cstr(name);
    v->is_array = is_array;
    v->lo = lo;
    v->hi = hi;
    long n = is_array ? (hi - lo + 1) : 1;
    if (n < 1) n = 1;
    v->data = calloc((size_t)n, sizeof(double));
    v->next = top->vars;
    top->vars = v;
    return v;
}

static void declare_names(Node *idents, Node *type) {
    int is_array = type && type->kind == N_ARRAY;
    long lo = 0, hi = 0;
    if (is_array) {
        lo = (long)type->child[0]->number;
        hi = (long)type->child[1]->number;
    }
    for (Node *id = idents; id; id = id->next) {
        make_var(id->text, is_array, lo, hi);
    }
}

static void declare_list(Node *decls) {
    for (Node *d = decls; d; d = d->next) {
        if (d->kind == N_VARDECL || d->kind == N_PARAM)
            declare_names(d->child[0], d->child[1]);
    }
}

static void free_frame(Frame *f) {
    Var *v = f->vars;
    while (v) {
        Var *n = v->next;
        free(v->name);
        free(v->data);
        free(v);
        v = n;
    }
    free(f);
}

static Node *find_sub(const char *name) {
    if (!unit) return NULL;
    for (Node *s = unit->child[2]; s; s = s->next) {
        if ((s->kind == N_FUNCTION || s->kind == N_PROCEDURE) &&
            s->child[0] && ieq(s->child[0]->text, name))
            return s;
    }
    return NULL;
}

static Val eval(Node *n);
static void exec(Node *n);

static double *lvalue(Node *var, int *ok) {
    *ok = 0;
    if (!var || var->kind != N_VAR || !var->child[0]) return NULL;
    const char *name = var->child[0]->text;
    Var *slot = find_var(name);
    if (!slot) {
        if (top->fun_name && ieq(top->fun_name, name) && !var->child[1]) {
            *ok = 2; /* function return */
            return NULL;
        }
        runtime_error(var->line, "unknown variable");
        return NULL;
    }
    if (slot->is_array) {
        if (!var->child[1]) {
            runtime_error(var->line, "array needs an index");
            return NULL;
        }
        long i = (long)eval(var->child[1]).num;
        if (i < slot->lo || i > slot->hi) {
            runtime_error(var->line, "array index out of range");
            return NULL;
        }
        *ok = 1;
        return &slot->data[i - slot->lo];
    }
    if (var->child[1]) {
        runtime_error(var->line, "not an array");
        return NULL;
    }
    *ok = 1;
    return &slot->data[0];
}

static Val num_val(double x) {
    Val v;
    v.is_str = 0;
    v.num = x;
    v.str = NULL;
    return v;
}

static Val str_val(const char *s) {
    Val v;
    v.is_str = 1;
    v.num = 0;
    v.str = s ? s : "";
    return v;
}

static int truth(Val v) {
    if (v.is_str) return v.str && v.str[0];
    return v.num != 0;
}

static double call_fun(Node *fn, Node *args) {
    double argv[64];
    int argc = 0;
    for (Node *arg = args; arg && argc < 64; arg = arg->next)
        argv[argc++] = eval(arg).num;

    Frame *frame = calloc(1, sizeof(Frame));
    frame->prev = top;
    frame->fun_name = fn->child[0] ? fn->child[0]->text : NULL;
    top = frame;

    declare_list(fn->child[1]);
    int k = 0;
    for (Node *p = fn->child[1]; p; p = p->next) {
        for (Node *id = p->child[0]; id; id = id->next) {
            Var *slot = find_var(id->text);
            if (slot && k < argc) slot->data[0] = argv[k++];
        }
    }

    Node *body = fn->child[3];
    if (body) {
        declare_list(body->child[1]);
        leave = 0;
        exec(body);
        leave = 0;
    }

    double ret = frame->ret;
    top = frame->prev;
    free_frame(frame);
    return ret;
}

static void do_read(Node *args) {
    for (Node *a = args; a; a = a->next) {
        int ok = 0;
        double *slot = lvalue(a, &ok);
        if (ok == 2) {
            runtime_error(a->line, "cannot read into a function name");
            continue;
        }
        if (!slot) continue;
        double x = 0;
        if (scanf("%lf", &x) != 1) {
            runtime_error(a->line, "expected a number on input");
            return;
        }
        *slot = x;
    }
}

static void do_write(Node *args, int newline) {
    for (Node *a = args; a; a = a->next) {
        Val v = eval(a);
        if (v.is_str) {
            fputs(v.str, stdout);
        } else if (v.num == (long)v.num) {
            printf("%ld", (long)v.num);
            fputc(' ', stdout);
        } else {
            printf("%g", v.num);
            fputc(' ', stdout);
        }
    }
    if (newline) fputc('\n', stdout);
    fflush(stdout);
}

static Val eval(Node *n) {
    Val z = num_val(0);
    if (!n || run_errors) return z;

    switch (n->kind) {
        case N_INT:
        case N_REAL:
        case N_BOOL:
            return num_val(n->number);
        case N_STRING:
            return str_val(n->text);
        case N_VAR: {
            int ok = 0;
            double *slot = lvalue(n, &ok);
            if (ok == 2) return num_val(top->ret);
            if (!slot) return z;
            return num_val(*slot);
        }
        case N_UNOP: {
            Val a = eval(n->child[0]);
            if (ieq(n->text, "not")) return num_val(!truth(a));
            if (ieq(n->text, "-")) return num_val(-a.num);
            return a;
        }
        case N_BINOP: {
            Val l = eval(n->child[0]);
            Val r = eval(n->child[1]);
            const char *op = n->text;
            if (ieq(op, "or")) return num_val(truth(l) || truth(r));
            if (ieq(op, "and")) return num_val(truth(l) && truth(r));
            if (ieq(op, "=")) return num_val(l.num == r.num);
            if (ieq(op, "<>")) return num_val(l.num != r.num);
            if (ieq(op, "<")) return num_val(l.num < r.num);
            if (ieq(op, "<=")) return num_val(l.num <= r.num);
            if (ieq(op, ">")) return num_val(l.num > r.num);
            if (ieq(op, ">=")) return num_val(l.num >= r.num);
            if (ieq(op, "+")) return num_val(l.num + r.num);
            if (ieq(op, "-")) return num_val(l.num - r.num);
            if (ieq(op, "*")) return num_val(l.num * r.num);
            if (ieq(op, "/")) return num_val(r.num == 0 ? 0 : l.num / r.num);
            if (ieq(op, "div")) return num_val(r.num == 0 ? 0 : (double)((long)l.num / (long)r.num));
            if (ieq(op, "mod")) return num_val(r.num == 0 ? 0 : (double)((long)l.num % (long)r.num));
            return z;
        }
        case N_CALL: {
            const char *name = n->child[0] ? n->child[0]->text : "";
            Node *fn = find_sub(name);
            if (!fn) {
                runtime_error(n->line, "unknown function");
                return z;
            }
            return num_val(call_fun(fn, n->child[1]));
        }
        default:
            return z;
    }
}

static void exec(Node *n) {
    for (; n && !run_errors && !leave; n = n->next) {
        switch (n->kind) {
            case N_EMPTY:
                break;
            case N_COMPOUND:
                exec(n->child[0]);
                break;
            case N_ASSIGN: {
                Val rhs = eval(n->child[1]);
                int ok = 0;
                double *slot = lvalue(n->child[0], &ok);
                if (ok == 2) {
                    top->ret = rhs.num;
                    top->has_ret = 1;
                } else if (slot) {
                    *slot = rhs.num;
                }
                break;
            }
            case N_CALL: {
                const char *name = n->child[0] ? n->child[0]->text : "";
                if (ieq(name, "read") || ieq(name, "readln"))
                    do_read(n->child[1]);
                else if (ieq(name, "write"))
                    do_write(n->child[1], 0);
                else if (ieq(name, "writeln"))
                    do_write(n->child[1], 1);
                else {
                    Node *fn = find_sub(name);
                    if (!fn) runtime_error(n->line, "unknown procedure");
                    else call_fun(fn, n->child[1]);
                }
                break;
            }
            case N_IF:
                if (truth(eval(n->child[0]))) exec(n->child[1]);
                else if (n->child[2]) exec(n->child[2]);
                break;
            case N_WHILE:
                while (!run_errors && !leave && truth(eval(n->child[0])))
                    exec(n->child[1]);
                break;
            case N_FOR: {
                if (!n->child[0]) break;
                Var *iv = find_var(n->child[0]->text);
                if (!iv) iv = make_var(n->child[0]->text, 0, 0, 0);
                long a = (long)eval(n->child[1]).num;
                long b = (long)eval(n->child[2]).num;
                int downto = n->text && ieq(n->text, "downto");
                if (downto) {
                    for (long i = a; !run_errors && !leave && i >= b; i--) {
                        iv->data[0] = (double)i;
                        exec(n->child[3]);
                    }
                } else {
                    for (long i = a; !run_errors && !leave && i <= b; i++) {
                        iv->data[0] = (double)i;
                        exec(n->child[3]);
                    }
                }
                break;
            }
            case N_RETURN:
                if (n->child[0]) {
                    top->ret = eval(n->child[0]).num;
                    top->has_ret = 1;
                }
                leave = 1;
                break;
            default:
                break;
        }
    }
}

int interpret(Node *program) {
    if (!program) return 1;
    run_errors = 0;
    leave = 0;
    unit = program;
    top = calloc(1, sizeof(Frame));
    declare_list(program->child[1]);
    exec(program->child[3]);
    free_frame(top);
    top = NULL;
    unit = NULL;
    return run_errors ? 1 : 0;
}
