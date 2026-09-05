# MiniPascal compiler (CSE303)

Lexer in **Flex**, parser in **Bison**, AST and driver in **C**. This phase only does lexical analysis and syntax analysis. It does not type-check and it does not generate code.

You do **not** need Ubuntu or WSL. Flex and Bison are the same lab tools on Windows. PowerShell can run them once they are installed.

If a teacher asks how it was built, see [GUIDE.md](GUIDE.md).

## Windows (PowerShell in VS Code)

Install the Windows ports of the course tools (once). Then close the terminal and open a new one so PATH updates:

```powershell
winget install -e --id WinFlexBison.win_flex_bison
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
```

If you already have `gcc` from Code::Blocks or Dev-C++, you only need WinFlexBison.

Then, in this folder:

```powershell
.\build.ps1
.\minipascal.exe samples\gcd.pas
.\minipascal.exe --tokens samples\gcd.pas
.\minipascal.exe samples\broken.pas
.\tests\run.ps1
```

Or Terminal → Run Task → **build** / **run gcd.pas**.

`build.ps1` is the Windows equivalent of the lab sequence: `bison -d`, `flex`, `gcc`.

## Linux / WSL (optional)

```bash
sudo apt install build-essential flex bison
make
./minipascal samples/gcd.pas
./minipascal --tokens samples/gcd.pas
./minipascal samples/broken.pas
make test
```

## Layout

```
compiler/lexer.l     Flex scanner
compiler/parser.y    Bison grammar and AST actions
compiler/ast.c       Tree nodes
compiler/dump.c      Print the tree
compiler/main.c      CLI
build.ps1            Windows build (PowerShell)
Makefile             Linux / WSL build
samples/             MiniPascal programs
```
