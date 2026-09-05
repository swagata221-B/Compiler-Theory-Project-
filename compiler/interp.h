#ifndef INTERP_H
#define INTERP_H

#include "ast.h"

/* Walk the parse tree and run the program. read/write use the terminal. */
int interpret(Node *program);

#endif
