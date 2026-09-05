# MiniPascal compiler (CSE303)

Same lab method as **calc.l / calc.y**: Flex, Bison, gcc, then type MiniPascal in the terminal.

Presentation 2 stops at **lexer + parser + parse tree**. You type the language. It prints the tree. It does not type-check and it does not generate code.

There are no leftover example programs (no gcd, no factorial). Tomorrow the input is the Pascal you type.

## Same three commands as the calc lab

In class you did:

```bash
bison -d calc.y
flex calc.l
gcc lex.yy.c calc.tab.c -o calc
./calc
```

Then: `Enter expression:` → type `2+3` → `Result = 5`.

This project is the same steps. Do them **inside the project folder**, not inside `C:\msys64\ucrt64\bin`.

```bash
cd ~/Downloads/Compiler-Theory-Project
sh ./build.sh
./minipascal.exe
```

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

```bash
./minipascal.exe --tokens
```

then type the same program to see tokens.

`samples/broken.pas` is only a syntax-error check. `--run` executes; that is past Presentation 2.
