#!/bin/sh
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MP="$ROOT/minipascal"
fail=0

ok() {
    file="$1"
    if ! "$MP" "$ROOT/$file" >/tmp/mp-out 2>/tmp/mp-err; then
        echo "FAIL $file (expected success)"
        cat /tmp/mp-err
        fail=1
    elif ! grep -q "Program" /tmp/mp-out; then
        echo "FAIL $file (no Program node)"
        cat /tmp/mp-out
        fail=1
    else
        echo "OK   $file"
    fi
}

bad() {
    file="$1"
    if "$MP" "$ROOT/$file" >/tmp/mp-out 2>/tmp/mp-err; then
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

ok samples/gcd.pas
ok samples/factorial.pas
ok samples/bubble.pas
bad samples/broken.pas

# 1..10 must be three tokens, not a real
"$MP" --tokens "$ROOT/samples/bubble.pas" >/tmp/mp-tok 2>/tmp/mp-err
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
