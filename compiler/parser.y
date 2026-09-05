%{
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

int yylex(void);
void yyerror(const char *msg);
%}

%code requires {
#include "ast.h"
}

%locations

%union {
    long ival;
    double real;
    char *str;
    Node *node;
}

%token PROGRAM VAR ARRAY OF INTEGER REAL BOOLEAN CHAR_TYPE
%token FUNCTION PROCEDURE BEGIN_KW END
%token IF THEN ELSE WHILE DO FOR TO DOWNTO
%token NOT AND OR DIV MOD
%token TRUE FALSE RETURN
%token ASSIGN EQ NEQ LT LTE GT GTE DOTDOT
%token PLUS MINUS STAR SLASH
%token LPAREN RPAREN LBRACKET RBRACKET
%token COMMA SEMI COLON DOT
%token <str> IDENT STRING_LIT
%token <ival> INTEGER_LIT
%token <real> REAL_LIT

%type <node> program ident decls decl_list decl type std_type
%type <node> ident_list subprograms subprogram params param_list param
%type <node> compound stmts stmt assign_or_call args expr
%type <node> variable

%left OR
%left AND
%nonassoc EQ NEQ LT LTE GT GTE
%left PLUS MINUS
%left STAR SLASH DIV MOD
%right NOT UMINUS UPLUS
%nonassoc THEN
%nonassoc ELSE

%start program

%%

program
    : PROGRAM ident SEMI decls subprograms compound DOT
        {
            $$ = node(N_PROGRAM, @1.first_line);
            $$->child[0] = $2;
            $$->child[1] = $4;
            $$->child[2] = $5;
            $$->child[3] = $6;
            ast_root = $$;
            YYACCEPT;
        }
    ;

ident
    : IDENT { $$ = ident($1, @1.first_line); free($1); }
    ;

ident_list
    : ident                 { $$ = $1; }
    | ident_list COMMA ident { $$ = append($1, $3); }
    ;

decls
    : /* empty */ { $$ = NULL; }
    | VAR decl_list { $$ = $2; }
    ;

decl_list
    : decl            { $$ = $1; }
    | decl_list decl  { $$ = append($1, $2); }
    ;

decl
    : ident_list COLON type SEMI
        {
            $$ = node(N_VARDECL, @1.first_line);
            $$->child[0] = $1;
            $$->child[1] = $3;
        }
    ;

type
    : std_type { $$ = $1; }
    | ARRAY LBRACKET INTEGER_LIT DOTDOT INTEGER_LIT RBRACKET OF std_type
        {
            $$ = node(N_ARRAY, @1.first_line);
            $$->child[0] = integer_lit($3, @3.first_line);
            $$->child[1] = integer_lit($5, @5.first_line);
            $$->child[2] = $8;
        }
    ;

std_type
    : INTEGER   { $$ = node(N_TYPE, @1.first_line); $$->text = copy_cstr("integer"); }
    | REAL      { $$ = node(N_TYPE, @1.first_line); $$->text = copy_cstr("real"); }
    | BOOLEAN   { $$ = node(N_TYPE, @1.first_line); $$->text = copy_cstr("boolean"); }
    | CHAR_TYPE { $$ = node(N_TYPE, @1.first_line); $$->text = copy_cstr("char"); }
    ;

subprograms
    : /* empty */              { $$ = NULL; }
    | subprograms subprogram SEMI { $$ = append($1, $2); }
    ;

subprogram
    : FUNCTION ident params COLON std_type SEMI decls compound
        {
            $$ = node(N_FUNCTION, @1.first_line);
            $$->child[0] = $2;
            $$->child[1] = $3;
            $$->child[2] = $5;
            $8->child[1] = $7;
            $$->child[3] = $8;
        }
    | PROCEDURE ident params SEMI decls compound
        {
            $$ = node(N_PROCEDURE, @1.first_line);
            $$->child[0] = $2;
            $$->child[1] = $3;
            $$->child[2] = $5;
            $$->child[3] = $6;
        }
    ;

params
    : /* empty */          { $$ = NULL; }
    | LPAREN RPAREN        { $$ = NULL; }
    | LPAREN param_list RPAREN { $$ = $2; }
    ;

param_list
    : param                  { $$ = $1; }
    | param_list SEMI param  { $$ = append($1, $3); }
    ;

param
    : ident_list COLON type
        {
            $$ = node(N_PARAM, @1.first_line);
            $$->child[0] = $1;
            $$->child[1] = $3;
        }
    ;

compound
    : BEGIN_KW stmts END
        {
            $$ = node(N_COMPOUND, @1.first_line);
            $$->child[0] = $2;
        }
    ;

stmts
    : stmt                 { $$ = $1; }
    | stmts SEMI stmt      { $$ = append($1, $3); }
    ;

stmt
    : /* empty */          { $$ = node(N_EMPTY, yylloc.first_line); }
    | assign_or_call       { $$ = $1; }
    | compound             { $$ = $1; }
    | IF expr THEN stmt
        {
            $$ = node(N_IF, @1.first_line);
            $$->child[0] = $2;
            $$->child[1] = $4;
        }
    | IF expr THEN stmt ELSE stmt
        {
            $$ = node(N_IF, @1.first_line);
            $$->child[0] = $2;
            $$->child[1] = $4;
            $$->child[2] = $6;
        }
    | WHILE expr DO stmt
        {
            $$ = node(N_WHILE, @1.first_line);
            $$->child[0] = $2;
            $$->child[1] = $4;
        }
    | FOR ident ASSIGN expr TO expr DO stmt
        {
            $$ = node(N_FOR, @1.first_line);
            $$->text = copy_cstr("to");
            $$->child[0] = $2;
            $$->child[1] = $4;
            $$->child[2] = $6;
            $$->child[3] = $8;
        }
    | FOR ident ASSIGN expr DOWNTO expr DO stmt
        {
            $$ = node(N_FOR, @1.first_line);
            $$->text = copy_cstr("downto");
            $$->child[0] = $2;
            $$->child[1] = $4;
            $$->child[2] = $6;
            $$->child[3] = $8;
        }
    | RETURN               { $$ = node(N_RETURN, @1.first_line); }
    | RETURN expr
        {
            $$ = node(N_RETURN, @1.first_line);
            $$->child[0] = $2;
        }
    | error                { $$ = node(N_EMPTY, yylloc.first_line); yyerrok; }
    ;

assign_or_call
    : ident ASSIGN expr
        {
            Node *v = node(N_VAR, $1->line);
            v->child[0] = $1;
            $$ = node(N_ASSIGN, @2.first_line);
            $$->child[0] = v;
            $$->child[1] = $3;
        }
    | ident LBRACKET expr RBRACKET ASSIGN expr
        {
            Node *v = node(N_VAR, $1->line);
            v->child[0] = $1;
            v->child[1] = $3;
            $$ = node(N_ASSIGN, @5.first_line);
            $$->child[0] = v;
            $$->child[1] = $6;
        }
    | ident
        {
            $$ = node(N_CALL, $1->line);
            $$->child[0] = $1;
        }
    | ident LPAREN args RPAREN
        {
            $$ = node(N_CALL, $1->line);
            $$->child[0] = $1;
            $$->child[1] = $3;
        }
    ;

variable
    : ident
        {
            $$ = node(N_VAR, $1->line);
            $$->child[0] = $1;
        }
    | ident LBRACKET expr RBRACKET
        {
            $$ = node(N_VAR, $1->line);
            $$->child[0] = $1;
            $$->child[1] = $3;
        }
    ;

args
    : /* empty */          { $$ = NULL; }
    | expr                 { $$ = $1; }
    | args COMMA expr      { $$ = append($1, $3); }
    ;

expr
    : expr OR expr         { $$ = binop("or", $1, $3, @2.first_line); }
    | expr AND expr        { $$ = binop("and", $1, $3, @2.first_line); }
    | expr EQ expr         { $$ = binop("=", $1, $3, @2.first_line); }
    | expr NEQ expr        { $$ = binop("<>", $1, $3, @2.first_line); }
    | expr LT expr         { $$ = binop("<", $1, $3, @2.first_line); }
    | expr LTE expr        { $$ = binop("<=", $1, $3, @2.first_line); }
    | expr GT expr         { $$ = binop(">", $1, $3, @2.first_line); }
    | expr GTE expr        { $$ = binop(">=", $1, $3, @2.first_line); }
    | expr PLUS expr       { $$ = binop("+", $1, $3, @2.first_line); }
    | expr MINUS expr      { $$ = binop("-", $1, $3, @2.first_line); }
    | expr STAR expr       { $$ = binop("*", $1, $3, @2.first_line); }
    | expr SLASH expr      { $$ = binop("/", $1, $3, @2.first_line); }
    | expr DIV expr        { $$ = binop("div", $1, $3, @2.first_line); }
    | expr MOD expr        { $$ = binop("mod", $1, $3, @2.first_line); }
    | NOT expr             { $$ = unop("not", $2, @1.first_line); }
    | MINUS expr %prec UMINUS { $$ = unop("-", $2, @1.first_line); }
    | PLUS expr %prec UPLUS   { $$ = unop("+", $2, @1.first_line); }
    | INTEGER_LIT          { $$ = integer_lit($1, @1.first_line); }
    | REAL_LIT             { $$ = real_lit($1, NULL, @1.first_line); }
    | TRUE                 { $$ = bool_lit(1, @1.first_line); }
    | FALSE                { $$ = bool_lit(0, @1.first_line); }
    | STRING_LIT           { $$ = string_lit($1, @1.first_line); free($1); }
    | variable             { $$ = $1; }
    | ident LPAREN args RPAREN
        {
            $$ = node(N_CALL, $1->line);
            $$->child[0] = $1;
            $$->child[1] = $3;
        }
    | LPAREN expr RPAREN   { $$ = $2; }
    ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "parse error at %d:%d: %s\n",
            yylloc.first_line, yylloc.first_column, msg);
    error_count++;
}
