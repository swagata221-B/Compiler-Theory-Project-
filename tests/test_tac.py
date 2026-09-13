"""TAC regression tests. Run with Python 3 after building MiniPascal."""
import operator
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / ('minipascal.exe' if (ROOT / 'minipascal.exe').exists() else 'minipascal')


def invoke(source, *flags):
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / 'test.pas'
        path.write_text(source)
        return subprocess.run([str(EXE), *flags, str(path)], text=True,
                              capture_output=True, timeout=10)


def program(body, declarations='x, y, i: integer;'):
    return f'program Test; var {declarations} begin {body} end.'


def execute_numeric_tac(output):
    """Independent execution of the numeric TAC subset, to check branch semantics."""
    lines = output.splitlines()
    labels = {line[:-1]: i for i, line in enumerate(lines) if line.endswith(':')}
    values, writes = {}, []
    operations = {
        '+': operator.add, '-': operator.sub, '*': operator.mul, '/': operator.truediv,
        'div': lambda a, b: int(a / b), 'mod': lambda a, b: a - int(a / b) * b,
        '=': operator.eq, '<>': operator.ne, '>': operator.gt, '<': operator.lt,
        '>=': operator.ge, '<=': operator.le, 'and': lambda a, b: bool(a) and bool(b),
        'or': lambda a, b: bool(a) or bool(b),
    }

    def value(token):
        if token in values:
            return values[token]
        if token in ('true', 'false'):
            return token == 'true'
        return float(token)

    pc = 0
    for _ in range(10000):
        if pc == len(lines):
            return writes
        parts = lines[pc].split()
        pc += 1
        if parts[0].endswith(':') or parts[0] == 'WRITELN':
            continue
        if parts[0] == 'GOTO':
            pc = labels[parts[1]]
        elif parts[0] == 'IF_FALSE':
            if not value(parts[1]):
                pc = labels[parts[3]]
        elif parts[0] == 'WRITE':
            writes.append(value(parts[1]))
        else:
            assert parts[1] == '=', parts
            rhs = parts[2:]
            if len(rhs) == 1:
                result = value(rhs[0])
            elif len(rhs) == 2:
                result = {'-': operator.neg, '+': operator.pos,
                          'not': operator.not_}[rhs[0]](value(rhs[1]))
            else:
                assert len(rhs) == 3, rhs
                result = operations[rhs[1]](value(rhs[0]), value(rhs[2]))
            values[parts[0]] = result
    raise AssertionError('TAC did not terminate')


class TacTests(unittest.TestCase):
    def tac(self, source):
        result = invoke(source, '--tac')
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout

    def test_slide_14(self):
        self.assertEqual(self.tac((ROOT / 'demo.pas').read_text()),
                         'n = 5\nt1 = n > 0\nIF_FALSE t1 GOTO L1\n'
                         't2 = n - 1\nn = t2\nL1:\n')

    def test_terminal_input(self):
        result = subprocess.run([str(EXE), '--tac'],
                                input=(ROOT / 'demo.pas').read_text(),
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('Enter MiniPascal program:', result.stdout)
        self.assertIn('IF_FALSE t1 GOTO L1', result.stdout)

    def test_numeric_semantics_match_ast_interpreter(self):
        cases = [
            'x := 2 + 3 * 4; writeln(x)',
            'x := 5; if x > 0 then x := x - 1; writeln(x)',
            'x := 0; if x > 0 then x := 7 else x := 9; writeln(x)',
            'x := 1; if x > 0 then begin if x = 1 then x := 2 else x := 3 end else x := 4; writeln(x)',
            'x := 0; while x < 4 do x := x + 1; writeln(x)',
            'x := 0; for i := 1 to 4 do x := x + i; writeln(x)',
            'x := 0; for i := 4 downto 1 do x := x + i; writeln(x)',
            'x := 0; for i := 4 to 1 do x := 99; writeln(x)',
            'x := 0; y := 3; for i := 1 to y do begin x := x + i; y := 0 end; writeln(x)',
            'x := -5 + 2; writeln(x); writeln(not false); writeln(true and false or true)',
            'x := 17 div 3; y := 17 mod 3; writeln(x); writeln(y)',
        ]
        for body in cases:
            with self.subTest(body=body):
                source = program(body)
                expected = invoke(source, '--run')
                self.assertEqual(expected.returncode, 0, expected.stderr)
                expected_numbers = [float(n) for n in expected.stdout.split()]
                self.assertEqual(execute_numeric_tac(self.tac(source)), expected_numbers)

    def test_temporary_names_and_case(self):
        source = program('T1 := 8; x := t1 + 2; writeln(x); writeln(T1)', 'T1, x: integer;')
        tac = self.tac(source)
        self.assertIn('t2 = t1 + 2', tac)
        self.assertEqual(execute_numeric_tac(tac), [10, 8])

    def test_io(self):
        tac = self.tac(program("read(x); writeln('It''s ', x)"))
        self.assertEqual(tac, "READ x\nWRITE 'It''s '\nWRITE x\nWRITELN\n")

    def test_unsupported_features_do_not_emit_partial_tac(self):
        sources = [
            program('x := 1', 'x: array [1..3] of integer;'),
            'program A; procedure P; begin end; begin end.',
            program('x := 1; return x'),
            program('x := 1; unknown(x)'),
            program('x := 1; x := writeln(2)'),
            program('x := 1; read(3)'),
        ]
        for source in sources:
            with self.subTest(source=source):
                result = invoke(source, '--tac')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('TAC error', result.stderr)
                self.assertEqual(result.stdout, '')

    def test_syntax_error_does_not_emit_tac(self):
        result = invoke('program A begin end.', '--tac')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('parse error', result.stderr)
        self.assertEqual(result.stdout, '')

    def test_conflicting_modes(self):
        for flag in ('--run', '--tokens'):
            result = invoke(program('x := 1'), '--tac', flag)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(result.stdout, '')

    def test_existing_modes(self):
        source = (ROOT / 'demo.pas').read_text()
        tree = invoke(source, '--tree')
        self.assertEqual(tree.returncode, 0, tree.stderr)
        self.assertIn('Program ', tree.stdout)
        self.assertIn('Ident Demo', tree.stdout)
        tokens = invoke(source, '--tokens')
        self.assertEqual(tokens.returncode, 0)
        self.assertEqual(len(re.findall(r'^\s*\d+:', tokens.stdout, re.M)), 25)


if __name__ == '__main__':
    unittest.main(verbosity=2)
