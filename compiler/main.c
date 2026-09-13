#include "ast.h"
#include "dump.h"
#include "interp.h"
#include "tac.h"
#include "parser.tab.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyin;
extern int yylex(void);
struct yy_buffer_state;
extern struct yy_buffer_state *yy_scan_bytes(const char *bytes, int length);
extern int yylex_destroy(void);
extern int yyparse(void);
extern int yylineno;
extern int yycolumn;
extern char *yytext;
extern YYLTYPE yylloc;

/* Keep source text available for diagnostics and AST annotations, including stdin. */
static char *source_text;
static char **source_lines;
static size_t source_line_count;

const char *source_line(int line) {
    return line > 0 && (size_t)line <= source_line_count ? source_lines[line - 1] : NULL;
}

void source_excerpt(FILE *out, int line, int column) {
    const char *text = source_line(line);
    if (!text) return;
    fprintf(out, "%d | %s\n", line, text);
    if (column > 0) {
        int width = snprintf(NULL, 0, "%d", line);
        fprintf(out, "%*s | ", width, "");
        for (int i = 1; i < column && text[i - 1]; ++i)
            fputc(text[i - 1] == '\t' ? '\t' : ' ', out);
        fputs("^\n", out);
    }
}

static int capture_source(FILE *input) {
    size_t size = 0, capacity = 4096;
    source_text = malloc(capacity);
    if (!source_text) return 0;
    int ch;
    while ((ch = fgetc(input)) != EOF) {
        if (size + 1 >= capacity) {
            char *grown = realloc(source_text, capacity * 2);
            if (!grown) return 0;
            source_text = grown;
            capacity *= 2;
        }
        source_text[size++] = (char)ch;
    }
    if (ferror(input)) return 0;
    source_text[size] = '\0';
    if (size > INT_MAX) return 0;
    /* Flex copies the bytes, so source_text can be split into display lines. */
    if (!yy_scan_bytes(source_text, (int)size)) return 0;
    source_line_count = 1;
    for (size_t i = 0; i < size; ++i) if (source_text[i] == '\n') ++source_line_count;
    source_lines = malloc(source_line_count * sizeof(*source_lines));
    if (!source_lines) return 0;
    size_t line = 0;
    source_lines[line++] = source_text;
    for (size_t i = 0; i < size; ++i) {
        if (source_text[i] == '\n') {
            source_text[i] = '\0';
            if (i > 0 && source_text[i - 1] == '\r') source_text[i - 1] = '\0';
            source_lines[line++] = source_text + i + 1;
        }
    }
    return 1;
}

static const char *token_name(int tok) {
    switch (tok) {
        case PROGRAM: return "PROGRAM";
        case VAR: return "VAR";
        case ARRAY: return "ARRAY";
        case OF: return "OF";
        case INTEGER: return "INTEGER";
        case REAL: return "REAL";
        case BOOLEAN: return "BOOLEAN";
        case CHAR_TYPE: return "CHAR";
        case FUNCTION: return "FUNCTION";
        case PROCEDURE: return "PROCEDURE";
        case BEGIN_KW: return "BEGIN";
        case END: return "END";
        case IF: return "IF";
        case THEN: return "THEN";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case DO: return "DO";
        case FOR: return "FOR";
        case TO: return "TO";
        case DOWNTO: return "DOWNTO";
        case NOT: return "NOT";
        case AND: return "AND";
        case OR: return "OR";
        case DIV: return "DIV";
        case MOD: return "MOD";
        case TRUE: return "TRUE";
        case FALSE: return "FALSE";
        case RETURN: return "RETURN";
        case IDENT: return "IDENT";
        case INTEGER_LIT: return "INTEGER_LIT";
        case REAL_LIT: return "REAL_LIT";
        case STRING_LIT: return "STRING_LIT";
        case ASSIGN: return "ASSIGN";
        case EQ: return "EQ";
        case NEQ: return "NEQ";
        case LT: return "LT";
        case LTE: return "LTE";
        case GT: return "GT";
        case GTE: return "GTE";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case STAR: return "STAR";
        case SLASH: return "SLASH";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case COMMA: return "COMMA";
        case SEMI: return "SEMI";
        case COLON: return "COLON";
        case DOT: return "DOT";
        case DOTDOT: return "DOTDOT";
        default: return "TOKEN";
    }
}

static int dump_tokens(void) {
    int tok;
    while ((tok = yylex()) != 0) {
        printf("%4d:%-3d  %-12s %s\n",
               yylloc.first_line, yylloc.first_column,
               token_name(tok), yytext);
        if (tok == IDENT || tok == STRING_LIT) free(yylval.str);
    }
    return error_count ? 1 : 0;
}

static void usage(const char *argv0) {
    fprintf(stderr,
            "Usage (same idea as the calc lab):\n"
            "  %s                      type MiniPascal, then see the parse tree\n"
            "  %s <file.pas>           parse a file, print the tree\n"
            "  %s --tokens [file]      print tokens\n"
            "  %s --tac [file.pas]     generate three-address code\n"
            "  %s --run <file.pas>     execute read/writeln (beyond Presentation 2)\n",
            argv0, argv0, argv0, argv0, argv0);
}

int main(int argc, char **argv) {
    int tokens_only = 0;
    int do_run = 0;
    int do_tac = 0;
    const char *file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) tokens_only = 1;
        else if (strcmp(argv[i], "--tree") == 0) { /* default; accepted for old scripts */ }
        else if (strcmp(argv[i], "--tac") == 0) do_tac = 1;
        else if (strcmp(argv[i], "--run") == 0) do_run = 1;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return 2;
        } else {
            file = argv[i];
        }
    }

    if (do_tac && (tokens_only || do_run)) {
        fprintf(stderr, "Use --tac separately from --tokens or --run\n");
        return 2;
    }

    if (file) {
        yyin = fopen(file, "r");
        if (!yyin) {
            perror(file);
            return 2;
        }
    } else {
        yyin = stdin;
        printf("MiniPascal compiler  (%s)\n", do_tac ? "three-address code" : "lexer + parser + tree");
        printf("Enter MiniPascal program:\n");
        fflush(stdout);
    }

    FILE *input = yyin;
    int captured = capture_source(input);
    if (file) fclose(input);
    else clearerr(stdin);
    if (!captured) {
        yylex_destroy();
        fprintf(stderr, "Cannot buffer source input for parsing\n");
        free(source_lines);
        free(source_text);
        return 2;
    }

    int status = 0;
    if (tokens_only) {
        if (file) printf("=== tokens (%s) ===\n", file);
        else printf("=== tokens ===\n");
        status = dump_tokens();
    } else {
        if (yyparse() != 0) status = 1;
        if (error_count) status = 1;
        if (do_tac) {
            if (ast_root && status == 0) status = emit_tac(ast_root, stdout);
        } else if (ast_root && do_run && status == 0) {
            status = interpret(ast_root);
        } else if (ast_root && status == 0) {
            printf("=== syntax tree (AST) ===\n");
            printf("Indentation shows children; [line N] refers to your Pascal source.\n");
            dump_tree(ast_root);
        }
        if (ast_root) free_tree(ast_root);
    }

    yylex_destroy();
    free(source_lines);
    free(source_text);
    return status;
}
