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

Semantic analysis and automatic IR emission are design-stage work for the current milestone. The proposed IR is three-address code, followed by a backend that lowers typed IR to C and uses GCC to generate a separate executable.

## Build and run

On Windows Git Bash, provide input as a **`.pas` file**. Do not type into the waiting prompt because Ctrl+D does not finish input there.

```bash
win_bison -d -o compiler/parser.tab.c compiler/parser.y
win_flex --wincompat -o compiler/lex.yy.c compiler/lexer.l
gcc -std=gnu11 -Icompiler -o minipascal.exe compiler/parser.tab.c compiler/lex.yy.c compiler/ast.c compiler/dump.c compiler/interp.c compiler/codegen.c compiler/main.c

./minipascal.exe demo.pas
```

Alternatively, run `sh ./build.sh` if `win_flex`, `win_bison`, and `gcc` are on `PATH`, then run `./minipascal.exe demo.pas`.

`demo.pas` is a short program. Replace it with another Pascal input, or run:

```bash
./minipascal.exe yourfile.pas
./minipascal.exe --tokens demo.pas
```

`samples/broken.pas` is a syntax-error check.

## Generate a standalone executable

The native backend currently supports numeric variables and arrays, assignment,
arithmetic and comparisons, `if`, `while`, and `write`/`writeln`. It generates C
from the AST and asks GCC to build a separate executable. It does not yet support
`read`, `for`, functions, procedures, string expressions, or full static type
checking; unsupported constructs produce a codegen error instead of a misleading
executable. Runtime `--run` remains the more complete interpreter path.

```bash
sh ./build.sh
./minipascal.exe --compile demo.pas -o demo-native.exe
./demo-native.exe
```

The command also leaves `demo-native.exe.c` so the generated C can be shown in
Presentation 3. The resulting `.exe` runs without `minipascal.exe` or `demo.pas`.
