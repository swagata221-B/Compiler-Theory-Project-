# MiniPascal compiler (CSE303)

Same lab method as **calc.l / calc.y**: Flex, Bison, gcc, then type input in the terminal.

Presentation 2 stops at **lexer + parser + parse tree**. You type MiniPascal. It prints the tree. It does not type-check and it does not generate code.

`gcd.pas` / `factorial.pas` are only practice files. Tomorrow you can type your own Pascal.

## Same three commands as the calc lab

In class you did:

```bash
bison -d calc.y
flex calc.l
gcc lex.yy.c calc.tab.c -o calc
./calc
```

Then: `Enter expression:` → type `2+3` → `Result = 5`.

This project is the same steps. Do them **inside the project folder**, not inside `C:\msys64\ucrt64\bin` (that folder is only where `gcc` / `flex` / `bison` live).

Git Bash or MSYS2 UCRT64, in `Compiler-Theory-Project`:

```bash
cd ~/Downloads/Compiler-Theory-Project
bison -d -o compiler/parser.tab.c compiler/parser.y
flex -o compiler/lex.yy.c compiler/lexer.l
gcc -Icompiler -o minipascal compiler/lex.yy.c compiler/parser.tab.c compiler/ast.c compiler/dump.c compiler/interp.c compiler/main.c
./minipascal.exe
```

Or one script: `sh ./build.sh` then `./minipascal.exe`.

## What you type (like the calculator)

```
MiniPascal compiler  (Presentation 2: lexer + parser + tree)
Enter MiniPascal program:
```

Type your program, finish with `end.`

```
program Demo;
var x: integer;
begin
  x := 1
end.
```

Output is the **parse tree** (this phase’s “Result =”).

A file works too:

```bash
./minipascal.exe samples/gcd.pas
./minipascal.exe --tokens samples/gcd.pas
```

`--run` executes `read` / `writeln`. That is past Presentation 2.

## Tools you already have

`C:\msys64\ucrt64\bin` has `gcc`, and lab already made `lex.yy.c` / `calc.tab.c` there. Put **this** project’s generated files in **this** folder, not next to `gcc`.
