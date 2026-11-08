import json
import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tests" / "qualification" / "linux_environment_report.py"


class LinuxEnvironmentReportTest(unittest.TestCase):
    def test_report_is_machine_readable_and_never_claims_support(self):
        result = subprocess.run((sys.executable, str(SCRIPT)), capture_output=True, text=True, check=False)
        report = json.loads(result.stdout)
        self.assertIn(report["system"], ("Linux", "Windows", "Darwin"))
        self.assertFalse(report["native_linux_claim"])
        expected_code = 0 if report["system"] == "Linux" else 2
        self.assertEqual(result.returncode, expected_code)
        self.assertIn("commands", report)


if __name__ == "__main__":
    unittest.main()
