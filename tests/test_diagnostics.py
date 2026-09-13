"""Source diagnostics and readable AST regression checks."""
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'minipascal.exe'
BAD = 'program Demo;\nvar x: integer;\nbegin\n  x := ;\nend.\n'
GOOD = BAD.replace('x := ;', 'x := 5;')

class DiagnosticsTests(unittest.TestCase):
    def invoke(self, source, mode, stdin=False, data=''):
        if stdin:
            return subprocess.run([str(EXE), mode], input=source, text=True,
                                  capture_output=True, timeout=5)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'input.pas'
            path.write_text(source)
            return subprocess.run([str(EXE), mode, str(path)], input=data,
                                  text=True, capture_output=True, timeout=5)

    def test_bad_assignment_blocks_tree_run_and_tac(self):
        for mode in ('--run', '--tac', '--tree'):
            with self.subTest(mode=mode):
                result = self.invoke(BAD, mode)
                self.assertEqual(result.returncode, 1)
                self.assertEqual(result.stdout, '')
                self.assertIn('4 |   x := ;\n  |        ^\nparse error at 4:8:', result.stderr)

    def test_terminal_source_error(self):
        result = self.invoke(BAD, '--run', stdin=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn('4 |   x := ;', result.stderr)
        self.assertNotIn('=== syntax tree', result.stdout)

    def test_tree_explains_statement_roles(self):
        result = self.invoke(GOOD, '--tree')
        self.assertEqual(result.returncode, 0)
        self.assertIn('Assignment  [line 4] | x := 5;', result.stdout)
        self.assertIn('Target', result.stdout)
        self.assertIn('Value', result.stdout)
        self.assertNotIn('Empty', result.stdout)

    def test_runtime_error_source(self):
        result = self.invoke(GOOD.replace('x := 5', 'x := 10 div 0'), '--run')
        self.assertEqual(result.returncode, 1)
        self.assertIn('4 |   x := 10 div 0;\nruntime error at line 4: division by zero', result.stderr)

    def test_file_source_keeps_runtime_input_separate(self):
        result = self.invoke(GOOD.replace('x := 5;', 'readln(x); writeln(x);'), '--run', data='7\n')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout.strip(), '7')

    def test_lexical_error_source(self):
        result = self.invoke(GOOD.replace('x := 5;', '@'), '--tokens')
        self.assertEqual(result.returncode, 1)
        self.assertIn('4 |   @', result.stderr)
        self.assertIn("unexpected character '@'", result.stderr)

if __name__ == '__main__':
    unittest.main(verbosity=2)
