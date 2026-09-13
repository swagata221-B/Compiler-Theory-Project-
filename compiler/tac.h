#ifndef TAC_H
#define TAC_H

#include "ast.h"
#include <stdio.h>

/* Lower the scalar AST to printable three-address code; return 0 on success. */
int emit_tac(Node *program, FILE *out);

#endif
