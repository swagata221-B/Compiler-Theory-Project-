# If someone asks how we made this

This is a MiniPascal **frontend** plus a small tree walker: Flex scanner, Bison parser, C AST, then `read` / `writeln` in the terminal. No type checker and no machine-code generation.

## One-minute answer

Flex turns `lexer.l` into a scanner (`lex.yy.c`). Bison turns `parser.y` into an LALR(1) parser (`parser.tab.c`). Those two generated files, plus a small AST in C, are linked into `minipascal`. The compiler reads a `.pas` file. `--tokens` prints the token stream. `--tree` prints the parse tree. With no flag it runs the tree: `read` waits in the terminal, `writeln` prints there. Parse errors include a line and column.

## Why Flex and Bison (not Node.js)

That is what a compiler course expects.

- **Flex** implements the regular-language part: identifiers, numbers, keywords, comments.
- **Bison** implements the context-free part: program, declarations, statements, expressions.
- We still write the rules and the AST actions. The tools only generate the tables.

Node.js was the wrong stack for this assignment. The project is C + Flex + Bison.

## How to run it in VS Code

Same three tools as the calc lab: `bison`, `flex`, `gcc`. Stay in the project folder (do not build inside `C:\msys64\ucrt64\bin`).

```bash
cd ~/Downloads/Compiler-Theory-Project
sh ./build.sh
./minipascal.exe
```

It prints `Enter MiniPascal program:` — type Pascal, end with `end.` — it prints the parse tree.

`samples/*.pas` are only examples. `--tokens` dumps the scanner. `--run` executes; that is not Presentation 2.

## What each file does

| File | Role |
| --- | --- |
| `compiler/lexer.l` | Regular expressions for tokens. Keywords are case-insensitive. |
| `compiler/parser.y` | CFG plus `%left` / `%right` precedence. Actions allocate AST nodes. |
| `compiler/ast.c` | Node structs: Program, VarDecl, While, Assign, … |
| `compiler/dump.c` | Prints the tree |
| `compiler/interp.c` | Walks the tree; `read` / `write` / `writeln` use the terminal |
| `compiler/main.c` | `minipascal file.pas` runs it; `--tree` and `--tokens` dump analysis |
| `build.sh` / `build.ps1` / `Makefile` | `bison -d`, `flex`, then `gcc` |

`build.sh`, `build.ps1`, or `make` produces `lex.yy.c` and `parser.tab.c`. We do not edit those generated files.

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

If you type:

```
program Demo;
var x: integer;
begin
  x := 1
end.
```

the tree looks like:

```
Program Demo
  VarDecl x : integer
  Assign x := 1
```

A later semantic pass would walk this tree. We do not do that yet.

## How we know it works

`sh ./tests/run.sh` or `make test` runs:

- typed MiniPascal must print a `Program` node
- `broken.pas` must fail and print a parse error
- `1..5` in an array type must tokenize as `DOTDOT`

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
