# MiniPascal compiler (CSE303)

Lexer in **Flex**, parser in **Bison**, AST and driver in **C**. This phase only does lexical analysis and syntax analysis. It does not type-check and it does not generate code.

Slides: `presentation/MiniPascal_CSE303_Presentation2.pptx`

If a teacher asks how it was built, see [GUIDE.md](GUIDE.md).

## Build and run (VS Code or any terminal)

Install `gcc`, `flex`, `bison`, and `make`.

On Ubuntu / WSL:

```bash
sudo apt install build-essential flex bison
make
./minipascal samples/gcd.pas
./minipascal --tokens samples/gcd.pas
./minipascal samples/broken.pas
make test
```

On Windows, use **WSL** and the same commands. Opening the folder in VS Code is enough; use the terminal or Run and Debug → **Build**, **Compile gcd.pas**.

You do not need Node.js and you do not need a cloud preview link.

## Layout

```
compiler/lexer.l     Flex scanner
compiler/parser.y    Bison grammar and AST actions
compiler/ast.c       Tree nodes
compiler/dump.c      Print the tree
compiler/main.c      CLI
samples/             MiniPascal programs
```
