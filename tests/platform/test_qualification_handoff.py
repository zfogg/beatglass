"""Regression coverage for the fail-closed issue #1 handoff contract."""

import subprocess
import sys
import unittest
from pathlib import Path


class QualificationHandoffTest(unittest.TestCase):
    def test_handoff_checker_accepts_current_exit_gate(self) -> None:
        root = Path(__file__).resolve().parents[2]
        result = subprocess.run(
            [sys.executable, str(root / "tests/platform/check_qualification_handoff.py")],
            cwd=root,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("qualification handoff covers", result.stdout)


if __name__ == "__main__":
    unittest.main()
