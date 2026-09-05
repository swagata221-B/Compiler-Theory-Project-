# If someone asks how we made this

This is a MiniPascal **frontend**: Flex scanner, Bison parser, C abstract syntax tree. We stop after syntax. No types, no code generation.

## One-minute answer

Flex turns `lexer.l` into a scanner (`lex.yy.c`). Bison turns `parser.y` into an LALR(1) parser (`parser.tab.c`). Those two generated files, plus a small AST in C, are linked into `minipascal`. The compiler reads a `.pas` file, prints tokens or a parse tree, and reports errors with a line and column.

## Why Flex and Bison (not Node.js)

That is what a compiler course expects.

- **Flex** implements the regular-language part: identifiers, numbers, keywords, comments.
- **Bison** implements the context-free part: program, declarations, statements, expressions.
- We still write the rules and the AST actions. The tools only generate the tables.

Node.js was the wrong stack for this assignment. The project is C + Flex + Bison.

## How to run it in VS Code

You do not need Linux. The course tools are Flex, Bison, and gcc. Those exist as Windows programs.

1. Install Flex, Bison, and gcc on Windows (once):

```powershell
winget install -e --id WinFlexBison.win_flex_bison
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
```

2. **File → Open Folder** on this project.
3. PowerShell:

```powershell
.\build.ps1
.\minipascal.exe samples\gcd.pas
.\minipascal.exe --tokens samples\gcd.pas
.\minipascal.exe samples\broken.pas
.\tests\run.ps1
```

Run Task has **build**, **run gcd.pas**, **show tokens**, and **test**.

On Linux the same steps are `make` and `./minipascal`.

## What each file does

| File | Role |
| --- | --- |
| `compiler/lexer.l` | Regular expressions for tokens. Keywords are case-insensitive. |
| `compiler/parser.y` | CFG plus `%left` / `%right` precedence. Actions allocate AST nodes. |
| `compiler/ast.c` | Node structs: Program, VarDecl, While, Assign, … |
| `compiler/dump.c` | Prints the tree |
| `compiler/main.c` | `minipascal file.pas` or `minipascal --tokens file.pas` |
| `build.ps1` / `Makefile` | `bison -d`, `flex`, then `gcc` |

`build.ps1` or `make` produces `lex.yy.c` and `parser.tab.c`. We do not edit those generated files.

## Lexer — what to say

`lexer.l` is a list of patterns. Flex picks the longest match.

- Keywords (`program`, `begin`, `while`, …) before identifiers.
- `{ … }`, `(* … *)`, and `//` comments are skipped.
- `1..10` is integer, `..`, integer. A real needs digits after the dot, so `1..10` cannot be a float.
- `:=`, `<>`, `<=`, `>=` are matched before the single-character operators.
- Strings use Pascal quotes; `''` is one quote.

Each token carries a line and column (`%locations` + `YY_USER_ACTION`).

## Parser — what to say

`parser.y` is the MiniPascal grammar. Bison builds an LALR table.

A program is `program Name; declarations subprograms begin … end.`

After an identifier:

- `:=` → assignment
- `[` … `] :=` → array assignment
- `(` … `)` → call
- otherwise → a call with no arguments (`writeln` style)

`if` / `then` / `else` uses `%nonassoc THEN` and `%nonassoc ELSE` so `else` binds to the nearest `if`.

Operators: `or`, `and`, comparisons, `+` `-`, `*` `/` `div` `mod`, then unary `not`.

On a syntax error Bison calls `yyerror` and we count it. `samples/broken.pas` is the demo of a rejected program.

## Tree

`samples/gcd.pas` becomes:

```
Program Gcd
  VarDecl a, b, t : integer
  Call read
  While b <> 0
    Assign t := b
    Assign b := a mod b
    Assign a := t
  Call writeln
```

A later semantic pass would walk this tree. We do not do that yet.

## How we know it works

`.\tests\run.ps1` (Windows) or `make test` (Linux) runs:

- `gcd.pas`, `factorial.pas`, `bubble.pas` must parse and print a `Program` node
- `broken.pas` must fail and print a parse error
- `1..5` in the array type must tokenize as `DOTDOT`

## Likely questions

**Why not write the scanner by hand?**  
We could. Flex is the course tool for regular tokens. The patterns in `lexer.l` are still ours.

**Why not a recursive-descent parser in C?**  
Bison is the course tool for CFGs. The grammar in `parser.y` is still ours. The actions build the AST.

**Does `x := y` fail if `y` is undeclared?**  
No. That is semantic analysis (next phase).

**Do you emit C or assembly?**  
Not in this phase.

**Did you copy a GitHub compiler?**  
No. This is a teaching MiniPascal grammar. The `.l`, `.y`, and AST are written for this project.
