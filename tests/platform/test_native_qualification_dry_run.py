import shutil
import subprocess
from pathlib import Path
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
