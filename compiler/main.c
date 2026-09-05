#include "ast.h"
#include "dump.h"
#include "interp.h"
#include "parser.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyin;
extern int yylex(void);
extern int yyparse(void);
extern int yylineno;
extern int yycolumn;
extern char *yytext;
extern YYLTYPE yylloc;

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
            "Usage:\n"
            "  %s <file.pas>           run the program (type input in this terminal)\n"
            "  %s --tree <file.pas>    print the parse tree\n"
            "  %s --tokens <file.pas>  print tokens\n",
            argv0, argv0, argv0);
}

static void trim_line(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ')) {
        s[--n] = '\0';
    }
}

int main(int argc, char **argv) {
    int tokens_only = 0;
    int tree_only = 0;
    const char *file = NULL;
    char pathbuf[1024];

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) tokens_only = 1;
        else if (strcmp(argv[i], "--tree") == 0) tree_only = 1;
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

    if (!file) {
        printf("MiniPascal file (example: samples/gcd.pas): ");
        fflush(stdout);
        if (!fgets(pathbuf, sizeof pathbuf, stdin)) return 2;
        trim_line(pathbuf);
        if (!pathbuf[0]) {
            usage(argv[0]);
            return 2;
        }
        file = pathbuf;
    }

    yyin = fopen(file, "r");
    if (!yyin) {
        perror(file);
        return 2;
    }

    int status = 0;
    if (tokens_only) {
        printf("=== tokens (%s) ===\n", file);
        status = dump_tokens();
    } else {
        if (yyparse() != 0) status = 1;
        if (error_count) status = 1;
        if (ast_root && tree_only) {
            printf("=== parse tree (%s) ===\n", file);
            dump_tree(ast_root);
        } else if (ast_root && status == 0) {
            fprintf(stderr, "=== running %s ===\n", file);
            fprintf(stderr, "If the program calls read, type numbers here and press Enter.\n");
            status = interpret(ast_root);
        }
        if (ast_root) free_tree(ast_root);
    }

    fclose(yyin);
    return status;
}
