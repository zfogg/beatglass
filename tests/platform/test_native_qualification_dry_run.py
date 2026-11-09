import shutil
import subprocess
import json
import sys
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).parents[2]


class NativeQualificationDryRunTest(unittest.TestCase):
    def test_windows_artifact_verifier_fixture(self):
        pwsh = shutil.which("pwsh") or shutil.which("powershell")
        if not pwsh:
            self.skipTest("PowerShell unavailable; native verifier dry-run not runnable")
        script = ROOT / "tests/platform/qualification_script_syntax.ps1"
        result = subprocess.run([pwsh, "-NoProfile", "-File", str(script)], cwd=ROOT,
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("qualification scripts:", result.stdout)

    def test_macos_verifier_shell_syntax(self):
        bash = shutil.which("bash")
        if not bash:
            self.skipTest("bash unavailable; macOS verifier syntax dry-run not runnable")
        script = ROOT / "tests/macos/verify_qualification_artifacts.sh"
        result = subprocess.run([bash, "-n", str(script)], cwd=ROOT,
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_linux_pending_handoff_contains_identity(self):
        runner = ROOT / "tests/pipewire/run_linux_native_qualification.py"
        with tempfile.TemporaryDirectory() as directory:
            summary = Path(directory) / "linux-summary.json"
            result = subprocess.run([sys.executable, str(runner), "--source", str(ROOT), "--summary", str(summary)],
                                    cwd=ROOT, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            payload = json.loads(summary.read_text(encoding="utf-8"))
            self.assertEqual(payload["runner_summary_schema"], "beatglass-native-runner-summary-v1")
            self.assertEqual(payload["handoff_status"], "pending")
            self.assertFalse(payload["native_execution_attempted"])
            self.assertIn("captured_at", payload)
            self.assertIn("environment", payload)
            self.assertTrue(payload["pending_reason"])
            self.assertIn("Linux host", payload["native_execution_blockers"])

    def test_cross_platform_scripts_mark_unrun_steps_explicitly(self):
        windows = (ROOT / "tests/windows/run_qualification.ps1").read_text(encoding="utf-8")
        macos = (ROOT / "tests/macos/run_qualification.sh").read_text(encoding="utf-8")
        self.assertIn("UNRUN", windows)
        self.assertIn("UNRUN", macos)
        self.assertIn("qualification-manifest.txt", windows)
        self.assertIn("qualification-manifest.txt", macos)
        self.assertIn("qualification-cases.tsv", macos)
