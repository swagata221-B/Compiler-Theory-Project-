#!/bin/sh
# Git Bash / MINGW / Linux
set -e
cd "$(dirname "$0")"
rm -rf presentation

if ! command -v bison >/dev/null || ! command -v flex >/dev/null || ! command -v gcc >/dev/null; then
    echo "Need bison, flex, and gcc on PATH."
    echo "Git Bash (MINGW): those come with MSYS2 / MinGW, not from WSL."
    exit 1
fi

bison -d -o compiler/parser.tab.c compiler/parser.y
flex -o compiler/lex.yy.c compiler/lexer.l
gcc -std=gnu11 -Wall -Wextra -Icompiler -o minipascal \
    compiler/parser.tab.c compiler/lex.yy.c \
    compiler/ast.c compiler/dump.c compiler/interp.c compiler/main.c

echo
echo "Built. Same as the calc lab: run it, then type MiniPascal."
echo "  ./minipascal.exe"
