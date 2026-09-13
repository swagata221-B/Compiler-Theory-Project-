# MiniPascal compiler (CSE303)

MiniPascal is a small Pascal-like compiler project built with Flex, Bison, and GCC. The current frontend recognizes source tokens, validates syntax, constructs an abstract syntax tree (AST), and reports source locations for errors. Presentation 2 covers the compiler architecture, lexer and parser design, AST construction, semantic-analysis design, and intermediate-representation strategy.

## Target platform and data representation

The current development target is a standard Windows 10/11 desktop or laptop with an x86-64 processor. WinFlexBison generates the lexer and parser, and 64-bit GCC builds the compiler executable.

A 64-bit target does not make every MiniPascal variable 64-bit. The proposed typed backend uses the following language-level representation:

| MiniPascal type | Proposed representation |
| --- | --- |
| `integer` | Signed 32-bit integer |
| `real` | IEEE-754 64-bit floating point |
| `boolean` | Logical `true` or `false` |
| `char` | 8-bit character |

The current prototype interpreter stores numeric runtime values in C `double` values. The final typed backend will distinguish integer and real storage explicitly.

## Lexer design

The lexer defines **52 named token categories**:

| Category | Count | Examples |
| --- | ---: | --- |
| Keywords | 28 | `PROGRAM`, `VAR`, `IF`, `WHILE` |
| Symbolic operators | 12 | `ASSIGN`, `PLUS`, `GT`, `DOTDOT` |
| Delimiters | 8 | `LPAREN`, `SEMI`, `COLON`, `DOT` |
| Identifiers and literals | 4 | `IDENT`, `INTEGER_LIT`, `REAL_LIT`, `STRING_LIT` |

These 52 entries are token *types*. The number of token instances depends on the input program. The included `demo.pas` produces 25 token instances before end-of-file.

Important lexer rules include:

```text
DIGIT  [0-9]
ID     [A-Za-z_][A-Za-z0-9_]*
```

Flex selects the longest matching lexeme. For equal-length matches, the earlier rule wins, so keyword rules appear before the general identifier rule. Whitespace and comments are skipped, keyword matching is case-insensitive, and token locations are recorded by line and column. Identifier and literal values travel to the parser through `yylval`.

## Parser and CFG

The Bison parser implements a context-free grammar with **20 nonterminals**, **75 production alternatives**, and `program` as the start symbol. Representative productions are:

```text
program
    : PROGRAM ident SEMI decls subprograms compound DOT

decl
    : ident_list COLON type SEMI

assign_or_call
    : ident ASSIGN expr

compound
    : BEGIN_KW stmts END
```

Operator precedence, from lower to higher, is `OR`, `AND`, relational operators, addition/subtraction, multiplication/division, and unary operators. For example, `2 + 3 * 4` is parsed as `2 + (3 * 4)`.

## From CFG to semantic validation

The CFG is not converted directly into semantic meaning. The frontend follows this sequence:

1. The lexer converts source characters into tokens.
2. The parser matches the tokens against CFG productions.
3. Bison semantic actions attached to matched productions construct AST nodes.
4. A semantic-analysis pass uses the AST and a symbol table to check declarations, scopes, and types.
5. A validated AST can then be lowered to three-address intermediate code.

For example, matching `ident ASSIGN expr` creates an `N_ASSIGN` AST node. This parser action constructs structure; the later semantic-analysis pass determines whether the identifier is declared and whether the expression type is compatible with it.

The `--tac` option now lowers the scalar AST to textual three-address code in `compiler/tac.c`. Static semantic analysis and a backend that lowers typed IR to C and builds a separate executable remain planned work. TAC generation currently assumes meaningful scalar input; it does not perform declaration or type checking.

## Build and run

On Windows Git Bash, provide input as a **`.pas` file**. Do not type into the waiting prompt because Ctrl+D does not finish input there.

```bash
win_bison -d -o compiler/parser.tab.c compiler/parser.y
win_flex --wincompat -o compiler/lex.yy.c compiler/lexer.l
gcc -std=gnu11 -Icompiler -o minipascal.exe compiler/parser.tab.c compiler/lex.yy.c compiler/ast.c compiler/dump.c compiler/interp.c compiler/tac.c compiler/main.c

./minipascal.exe demo.pas
```

Alternatively, run `sh ./build.sh` if `win_flex`, `win_bison`, and `gcc` are on `PATH`, then run `./minipascal.exe demo.pas`.

`demo.pas` is a short program. Replace it with another Pascal input, or run:

```bash
./minipascal.exe yourfile.pas
./minipascal.exe --tokens demo.pas
```

`samples/broken.pas` is a syntax-error check.


## Three-address code (TAC)

Build with `build.ps1`, `build.sh`, or `make`, then run in PowerShell:

```powershell
.\minipascal.exe --tac demo.pas
```

Output matching the Presentation 2 example (with `L1` as the end label):

```text
n = 5
t1 = n > 0
IF_FALSE t1 GOTO L1
t2 = n - 1
n = t2
L1:
```

Use `--tac` without a filename to enter Pascal in the terminal. Finish input
with the terminal's EOF sequence (PowerShell: Ctrl+Z, then Enter). File input
is recommended for demonstrations. Syntax errors prevent TAC emission.

`parser.y` builds the AST. `main.c` calls `emit_tac()` from `compiler/tac.c`.
Search that file for `expression` (temporary values), `N_ASSIGN` (assignments),
`N_IF` (branches), `N_WHILE` / `N_FOR` (loops), or `emit_tac` (entry point).

Supported: scalar literals/variables, arithmetic and comparisons, unary and
Boolean operations, assignment, if/else, while, for/to/downto, and basic
read/readln/write/writeln statements. Boolean operands are evaluated eagerly,
as in the prototype interpreter. READ consumes a numeric value; WRITELN adds
a newline. Variable names are normalized to lowercase because Pascal is
case-insensitive. Generated temporary names skip source identifiers.

Arrays, user subprograms, return statements, and calls inside expressions
produce an explicit TAC error. No partial TAC is emitted. This is untyped
textual IR for demonstration; `--run` still executes the AST, and `--tac`
does not create a separate executable. Static type checking is not included.

Additional TAC regression checks (Python 3): `py -3 tests/test_tac.py`.
