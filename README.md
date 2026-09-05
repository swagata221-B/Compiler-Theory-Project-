# MiniPascal compiler (CSE303)

Flex + Bison + gcc, same as the calc lab. Presentation 2 is lexer, parser, and parse tree only.

On Windows Git Bash, give Pascal as a **`.pas` file**. Do not type into the waiting prompt (Ctrl+D does not finish input there).

```bash
win_bison -d -o compiler/parser.tab.c compiler/parser.y
win_flex --wincompat -o compiler/lex.yy.c compiler/lexer.l
gcc -std=gnu11 -Icompiler -o minipascal.exe compiler/parser.tab.c compiler/lex.yy.c compiler/ast.c compiler/dump.c compiler/interp.c compiler/main.c

./minipascal.exe demo.pas
```

Or `sh ./build.sh` if `win_flex`, `win_bison`, and `gcc` are on PATH, then `./minipascal.exe demo.pas`.

`demo.pas` is a short program. Replace it with tomorrow’s Pascal, or:

```bash
./minipascal.exe yourfile.pas
./minipascal.exe --tokens demo.pas
```

`samples/broken.pas` is a syntax-error check.
