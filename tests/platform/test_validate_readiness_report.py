import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("validate_report", Path(__file__).with_name("validate_readiness_report.py"))
module = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(module)


class ReadinessReportContractTest(unittest.TestCase):
    def test_incomplete_fixture_has_valid_conservative_contract(self):
        report = __import__("json").loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        self.assertEqual(module.validate(report), [])

    def test_ready_report_cannot_hide_reason_codes(self):
        report = {"readiness_report_schema": 1, "support_language": "experimental", "native_qualification_required": True,
                  "exit_gate_status": "READY", "exit_gate_reason_codes": ["scenario"],
                  "platform_support_status": [{"platform": p} for p in ("windows", "macos", "linux")]}
        self.assertIn("READY report cannot contain", "\n".join(module.validate(report)))
