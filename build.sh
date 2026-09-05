#!/bin/sh
# Git Bash: gcc from MinGW, Flex/Bison from WinFlexBison (win_flex / win_bison)
set -e
cd "$(dirname "$0")"
rm -rf presentation

for d in \
    /c/MinGW/bin \
    /c/mingw64/bin \
    /c/msys64/ucrt64/bin \
    /c/msys64/mingw64/bin \
    /c/msys64/usr/bin \
    /c/Users/swaga/AppData/Local/Microsoft/WinGet/Packages/WinFlexBison.win_flex_bison_Microsoft.Winget.Source_8wekyb3d8bbwe
do
    if [ -d "$d" ]; then
        PATH="$d:$PATH"
    fi
done
# any other WinFlexBison winget folder
for d in /c/Users/*/AppData/Local/Microsoft/WinGet/Packages/WinFlexBison.win_flex_bison_*; do
    if [ -d "$d" ]; then
        PATH="$d:$PATH"
    fi
done
export PATH

BISON=$(command -v bison || command -v win_bison || true)
FLEX=$(command -v flex || command -v win_flex || true)
CC=$(command -v gcc || true)

if [ -z "$BISON" ] || [ -z "$FLEX" ] || [ -z "$CC" ]; then
    echo "Need gcc plus flex/win_flex plus bison/win_bison."
    echo "gcc:    ${CC:-MISSING}"
    echo "flex:   ${FLEX:-MISSING}"
    echo "bison:  ${BISON:-MISSING}"
    echo
    echo "You already have WinFlexBison. In Git Bash run:"
    echo "  export PATH=\"/c/Users/\$USER/AppData/Local/Microsoft/WinGet/Packages/WinFlexBison.win_flex_bison_Microsoft.Winget.Source_8wekyb3d8bbwe:/c/MinGW/bin:\$PATH\""
    echo "  which gcc win_flex win_bison"
    exit 1
fi

echo "bison: $BISON"
echo "flex:  $FLEX"
echo "gcc:   $CC"

"$BISON" -d -o compiler/parser.tab.c compiler/parser.y

flexname=$(basename "$FLEX")
flexname=${flexname%.exe}
if [ "$flexname" = "win_flex" ]; then
    "$FLEX" --wincompat -o compiler/lex.yy.c compiler/lexer.l
else
    "$FLEX" -o compiler/lex.yy.c compiler/lexer.l
fi

"$CC" -std=gnu11 -Wall -Wextra -Icompiler -o minipascal \
    compiler/parser.tab.c compiler/lex.yy.c \
    compiler/ast.c compiler/dump.c compiler/interp.c compiler/main.c

echo
echo "Built. Run:"
if [ -f minipascal.exe ]; then
    echo "  ./minipascal.exe"
else
    echo "  ./minipascal"
fi
