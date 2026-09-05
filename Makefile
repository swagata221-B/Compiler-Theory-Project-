CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -Icompiler
LEX     = flex
YACC    = bison

OBJS = compiler/parser.tab.o compiler/lex.yy.o compiler/ast.o compiler/dump.o compiler/interp.o compiler/main.o

.PHONY: all clean test tokens

all: minipascal

minipascal: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

compiler/parser.tab.c compiler/parser.tab.h: compiler/parser.y
	$(YACC) -d -o compiler/parser.tab.c compiler/parser.y

compiler/lex.yy.c: compiler/lexer.l compiler/parser.tab.h
	$(LEX) -o compiler/lex.yy.c compiler/lexer.l

compiler/parser.tab.o: compiler/parser.tab.c compiler/ast.h
	$(CC) $(CFLAGS) -c -o $@ compiler/parser.tab.c

compiler/lex.yy.o: compiler/lex.yy.c compiler/parser.tab.h compiler/ast.h
	$(CC) $(CFLAGS) -c -o $@ compiler/lex.yy.c

compiler/ast.o: compiler/ast.c compiler/ast.h
	$(CC) $(CFLAGS) -c -o $@ compiler/ast.c

compiler/dump.o: compiler/dump.c compiler/dump.h compiler/ast.h
	$(CC) $(CFLAGS) -c -o $@ compiler/dump.c

compiler/interp.o: compiler/interp.c compiler/interp.h compiler/ast.h
	$(CC) $(CFLAGS) -c -o $@ compiler/interp.c

compiler/main.o: compiler/main.c compiler/ast.h compiler/dump.h compiler/interp.h compiler/parser.tab.h
	$(CC) $(CFLAGS) -c -o $@ compiler/main.c

test: minipascal
	./tests/run.sh

tokens: minipascal
	printf 'program A;\nvar a: array [1..5] of integer;\nbegin\nend.\n' | ./minipascal --tokens

clean:
	rm -f minipascal minipascal.exe compiler/parser.tab.c compiler/parser.tab.h compiler/lex.yy.c $(OBJS)
