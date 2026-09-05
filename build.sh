#!/bin/sh
# Git Bash / MINGW / Linux — same bison, flex, gcc as the calc lab
set -e
cd "$(dirname "$0")"
rm -rf presentation

# Lab tools live in MSYS2. Git Bash does not put them on PATH by itself.
for d in \
    /c/msys64/ucrt64/bin \
    /c/msys64/mingw64/bin \
    /c/msys64/usr/bin \
    /c/mingw64/bin
do
    if [ -d "$d" ]; then
        PATH="$d:$PATH"
    fi
done
export PATH

need=""
command -v bison >/dev/null || need="$need bison"
command -v flex  >/dev/null || need="$need flex"
command -v gcc   >/dev/null || need="$need gcc"

if [ -n "$need" ]; then
    echo "Missing:$need"
    echo
    echo "In this same Git Bash window, run:"
    echo "  export PATH=\"/c/msys64/ucrt64/bin:/c/msys64/usr/bin:\$PATH\""
    echo "  which gcc flex bison"
    echo "  sh ./build.sh"
    echo
    echo "If which still fails, open 'MSYS2 UCRT64' from the Start menu"
    echo "and install the lab tools once:"
    echo "  pacman -S --needed base-devel gcc flex bison"
    exit 1
fi

echo "bison: $(command -v bison)"
echo "flex:  $(command -v flex)"
echo "gcc:   $(command -v gcc)"

bison -d -o compiler/parser.tab.c compiler/parser.y
flex -o compiler/lex.yy.c compiler/lexer.l
gcc -std=gnu11 -Wall -Wextra -Icompiler -o minipascal \
    compiler/parser.tab.c compiler/lex.yy.c \
    compiler/ast.c compiler/dump.c compiler/interp.c compiler/main.c

echo
echo "Built. Now run:"
echo "  ./minipascal.exe"
if [ -f minipascal.exe ]; then
    echo "(created minipascal.exe)"
elif [ -f minipascal ]; then
    echo "(created minipascal — run ./minipascal)"
fi
