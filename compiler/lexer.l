%{
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#ifdef _WIN32
#define YY_NO_UNISTD_H 1
#endif
#include "ast.h"
#include "parser.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yycolumn = 1;
static int after_program_dot = 0;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

static char *dup_text(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static char *unquote_pascal(const char *text) {
    /* 'hello' or 'it''s' — drop the outer quotes and collapse doubled quotes */
    size_t n = strlen(text);
    char *out = malloc(n);
    size_t j = 0;
    for (size_t i = 1; i + 1 < n; i++) {
        if (text[i] == '\'' && text[i + 1] == '\'') {
            out[j++] = '\'';
            i++;
        } else {
            out[j++] = text[i];
        }
    }
    out[j] = '\0';
    return out;
}
%}

%option noyywrap nounput noinput yylineno case-insensitive

%x COMMENT BRACE

DIGIT   [0-9]
ID      [A-Za-z_][A-Za-z0-9_]*

%%

[ \t\r]+                { }
\n                      {
                          yycolumn = 1;
                          /* like the calc lab: after end. a newline finishes input */
                          if (after_program_dot) {
                              after_program_dot = 0;
                              return 0;
                          }
                        }

"//".*                  { }

"{"                     { BEGIN(BRACE); }
<BRACE>[^}\n]+          { }
<BRACE>\n               { yycolumn = 1; }
<BRACE>"}"              { BEGIN(INITIAL); }
<BRACE><<EOF>>          {
                          fprintf(stderr, "lex error at %d:%d: unterminated comment\n",
                                  yylineno, yycolumn);
                          error_count++;
                          return 0;
                        }

"(*"                    { BEGIN(COMMENT); }
<COMMENT>[^*\n]+        { }
<COMMENT>"*"+[^*)\n]*   { }
<COMMENT>\n             { yycolumn = 1; }
<COMMENT>"*"+")"        { BEGIN(INITIAL); }
<COMMENT><<EOF>>        {
                          fprintf(stderr, "lex error at %d:%d: unterminated comment\n",
                                  yylineno, yycolumn);
                          error_count++;
                          return 0;
                        }

"program"               { return PROGRAM; }
"var"                   { return VAR; }
"array"                 { return ARRAY; }
"of"                    { return OF; }
"integer"               { return INTEGER; }
"real"                  { return REAL; }
"boolean"               { return BOOLEAN; }
"char"                  { return CHAR_TYPE; }
"function"              { return FUNCTION; }
"procedure"             { return PROCEDURE; }
"begin"                 { return BEGIN_KW; }
"end"                   { return END; }
"if"                    { return IF; }
"then"                  { return THEN; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }
"do"                    { return DO; }
"for"                   { return FOR; }
"to"                    { return TO; }
"downto"                { return DOWNTO; }
"not"                   { return NOT; }
"and"                   { return AND; }
"or"                    { return OR; }
"div"                   { return DIV; }
"mod"                   { return MOD; }
"true"                  { return TRUE; }
"false"                 { return FALSE; }
"return"                { return RETURN; }

":="                    { return ASSIGN; }
"<>"                    { return NEQ; }
"<="                    { return LTE; }
">="                    { return GTE; }
".."                    { return DOTDOT; }

"="                     { return EQ; }
"<"                     { return LT; }
">"                     { return GT; }
"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return STAR; }
"/"                     { return SLASH; }
"("                     { return LPAREN; }
")"                     { return RPAREN; }
"["                     { return LBRACKET; }
"]"                     { return RBRACKET; }
","                     { return COMMA; }
";"                     { return SEMI; }
":"                     { return COLON; }
"."                     { after_program_dot = 1; return DOT; }

{DIGIT}+\.{DIGIT}+([eE][+-]?{DIGIT}+)? {
                          yylval.real = strtod(yytext, NULL);
                          return REAL_LIT;
                        }

{DIGIT}+                {
                          yylval.ival = strtol(yytext, NULL, 10);
                          return INTEGER_LIT;
                        }

{ID}                    {
                          yylval.str = dup_text(yytext);
                          return IDENT;
                        }

\'([^'\n]|\'\')*\'      {
                          yylval.str = unquote_pascal(yytext);
                          return STRING_LIT;
                        }

\'([^'\n]|\'\')*        {
                          fprintf(stderr, "lex error at %d:%d: unterminated string\n",
                                  yylineno, yycolumn - yyleng);
                          error_count++;
                          yylval.str = dup_text("");
                          return STRING_LIT;
                        }

.                       {
                          fprintf(stderr, "lex error at %d:%d: unexpected character '%s'\n",
                                  yylineno, yycolumn - yyleng, yytext);
                          error_count++;
                        }

%%
