# MiniPascal compiler (CSE303)

Lexer in **Flex**, parser in **Bison**, AST and a small runner in **C**.

You type a MiniPascal file in the **terminal**. The program can `read` numbers from that same terminal and `writeln` the result there. There is no extra window.

`presentation/` is not part of this project. If that folder is still on disk, delete it.

## Run in VS Code (Git Bash / MINGW64)

This is the terminal that looks like `swaga@Muninn MINGW64`.

Do **not** use PowerShell backslashes. In Git Bash, `.\minipascal.exe` becomes a broken command.

**1.** File → Open Folder → `Compiler-Theory-Project`

**2.** View → Terminal. You should already be in the project:

```
swaga@Muninn MINGW64 ~/Downloads/Compiler-Theory-Project
```

If not:

```bash
cd ~/Downloads/Compiler-Theory-Project
```

**3.** Build (only if `minipascal.exe` is missing or you changed the compiler):

```bash
sh ./build.sh
```

**4.** Run. Use a **dot-slash** and **forward slashes**:

```bash
./minipascal.exe samples/gcd.pas
```

**5.** When it asks for input, type numbers and press Enter:

```
48 18
```

You should see:

```
gcd = 6
```

Other programs:

```bash
./minipascal.exe samples/factorial.pas
# then type: 5
# output: 120

./minipascal.exe samples/bubble.pas
# then type: 5 1 4 2 3
# output: 1 2 3 4 5

./minipascal.exe --tree samples/gcd.pas
./minipascal.exe --tokens samples/gcd.pas
./minipascal.exe samples/broken.pas
```

No file argument: the compiler asks `MiniPascal file:` — type `samples/gcd.pas`, then the numbers.

## PowerShell (only if the prompt starts with `PS C:\`)

```powershell
.\build.ps1
.\minipascal.exe samples\gcd.pas
```

## Linux

```bash
sudo apt install build-essential flex bison
make
./minipascal samples/gcd.pas
make test
```

## Layout

```
compiler/lexer.l     Flex scanner
compiler/parser.y    Bison grammar
compiler/ast.c       Tree nodes
compiler/interp.c    Runs the tree (read / writeln)
compiler/main.c      Terminal driver
samples/             MiniPascal programs
build.sh             Git Bash build
build.ps1            PowerShell build
```
