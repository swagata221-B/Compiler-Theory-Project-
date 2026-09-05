#!/bin/sh
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ -f "$ROOT/minipascal.exe" ]; then
    MP="$ROOT/minipascal.exe"
else
    MP="$ROOT/minipascal"
fi
fail=0

ok_tree() {
    file="$1"
    if ! "$MP" --tree "$ROOT/$file" >/tmp/mp-out 2>/tmp/mp-err; then
        echo "FAIL $file (expected success)"
        cat /tmp/mp-err
        fail=1
    elif ! grep -q "Program" /tmp/mp-out; then
        echo "FAIL $file (no Program node)"
        cat /tmp/mp-out
        fail=1
    else
        echo "OK   $file (tree)"
    fi
}

bad() {
    file="$1"
    if "$MP" --tree "$ROOT/$file" >/tmp/mp-out 2>/tmp/mp-err; then
        echo "FAIL $file (expected syntax error)"
        fail=1
    elif ! grep -qi "parse error\|lex error" /tmp/mp-err; then
        echo "FAIL $file (no error message)"
        cat /tmp/mp-err
        fail=1
    else
        echo "OK   $file (rejected)"
    fi
}

ok_tree samples/gcd.pas
ok_tree samples/factorial.pas
ok_tree samples/bubble.pas
bad samples/broken.pas

"$MP" --tokens "$ROOT/samples/bubble.pas" >/tmp/mp-tok 2>/tmp/mp-err
if ! grep -q "DOTDOT" /tmp/mp-tok; then
    echo "FAIL array range is not tokenized as .."
    fail=1
else
    echo "OK   array range tokens"
fi

printf 'program Demo;\nbegin\nend.\n' | "$MP" >/tmp/mp-in 2>/tmp/mp-err || true
if ! grep -q "Program" /tmp/mp-in; then
    echo "FAIL typed MiniPascal from stdin"
    cat /tmp/mp-in /tmp/mp-err
    fail=1
else
    echo "OK   typed MiniPascal from stdin"
fi

printf '48 18\n' | "$MP" --run "$ROOT/samples/gcd.pas" >/tmp/mp-run 2>/tmp/mp-err || true
if ! grep -q "gcd = 6" /tmp/mp-run; then
    echo "FAIL gcd run"
    cat /tmp/mp-run /tmp/mp-err
    fail=1
else
    echo "OK   gcd run"
fi

printf '5\n' | "$MP" --run "$ROOT/samples/factorial.pas" >/tmp/mp-run 2>/tmp/mp-err || true
if ! grep -q "120" /tmp/mp-run; then
    echo "FAIL factorial run"
    cat /tmp/mp-run /tmp/mp-err
    fail=1
else
    echo "OK   factorial run"
fi

printf '5 1 4 2 3\n' | "$MP" --run "$ROOT/samples/bubble.pas" >/tmp/mp-run 2>/tmp/mp-err || true
if ! grep -q "1 2 3 4 5" /tmp/mp-run; then
    echo "FAIL bubble run"
    cat /tmp/mp-run /tmp/mp-err
    fail=1
else
    echo "OK   bubble run"
fi

if [ "$fail" -ne 0 ]; then
    exit 1
fi
echo "All tests passed."
