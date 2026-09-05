#!/bin/sh
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ -f "$ROOT/minipascal.exe" ]; then
    MP="$ROOT/minipascal.exe"
else
    MP="$ROOT/minipascal"
fi
fail=0

printf 'program Demo;\nvar x: integer;\nbegin\n  x := 1\nend.\n' | "$MP" >/tmp/mp-out 2>/tmp/mp-err || true
if ! grep -q "Program" /tmp/mp-out; then
    echo "FAIL typed MiniPascal (expected parse tree)"
    cat /tmp/mp-out /tmp/mp-err
    fail=1
else
    echo "OK   typed MiniPascal"
fi

if "$MP" "$ROOT/samples/broken.pas" >/tmp/mp-out 2>/tmp/mp-err; then
    echo "FAIL broken.pas (expected syntax error)"
    fail=1
elif ! grep -qi "parse error\|lex error" /tmp/mp-err; then
    echo "FAIL broken.pas (no error message)"
    cat /tmp/mp-err
    fail=1
else
    echo "OK   broken.pas (rejected)"
fi

printf 'program A;\nvar a: array [1..5] of integer;\nbegin\nend.\n' | "$MP" --tokens >/tmp/mp-tok 2>/tmp/mp-err
if ! grep -q "DOTDOT" /tmp/mp-tok; then
    echo "FAIL array range is not tokenized as .."
    fail=1
else
    echo "OK   array range tokens"
fi

if [ "$fail" -ne 0 ]; then
    exit 1
fi
echo "All tests passed."
