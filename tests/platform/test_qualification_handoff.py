"""Regression coverage for the fail-closed issue #1 handoff contract."""

import subprocess
import sys
import unittest
import importlib.util
from pathlib import Path

_checker_path = Path(__file__).with_name("check_qualification_handoff.py")
_spec = importlib.util.spec_from_file_location("check_qualification_handoff", _checker_path)
assert _spec and _spec.loader
_checker = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_checker)
PLATFORM_NATIVE_EVIDENCE = _checker.PLATFORM_NATIVE_EVIDENCE


class QualificationHandoffTest(unittest.TestCase):
    def test_each_platform_contract_names_native_backend_and_ui_observations(self) -> None:
        root = Path(__file__).resolve().parents[2]
        docs = (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8").lower()
        rows = {
            line.strip("|").split("|")[0].strip().lower(): line.lower()
            for line in docs.splitlines()
            if line.lower().startswith(("| windows |", "| macos |", "| linux |"))
        }
        self.assertEqual(set(rows), {platform.lower() for platform in PLATFORM_NATIVE_EVIDENCE})
        for platform, requirements in PLATFORM_NATIVE_EVIDENCE.items():
            for requirement in requirements:
                self.assertIn(requirement.lower(), rows[platform.lower()])

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

    def test_handoff_docs_describe_versioned_report_contract(self) -> None:
        root = Path(__file__).resolve().parents[2]
        docs = (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
        for phrase in ("readiness_report_schema: 1", "exit_gate_status", "exit_gate_reason_codes", "platform_support_status", "native_qualification_required"):
            self.assertIn(phrase, docs)

    def test_current_status_has_one_row_per_platform(self) -> None:
        root = Path(__file__).resolve().parents[2]
        rows = [line for line in (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8").splitlines()
                if line.startswith(("| Windows |", "| macOS |", "| Linux |"))]
        self.assertEqual(len(rows), len({row.split("|")[1].strip() for row in rows}))


if __name__ == "__main__":
    unittest.main()
