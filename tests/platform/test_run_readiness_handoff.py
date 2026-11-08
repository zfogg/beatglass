import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("handoff", Path(__file__).with_name("run_readiness_handoff.py"))
handoff = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(handoff)


class ReadinessHandoffTest(unittest.TestCase):
    def test_generates_and_validates_conservative_report(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "readiness.json"
            self.assertEqual(handoff.run(bundles, output), [])
            report = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(report["exit_gate_status"], "PENDING")
            self.assertTrue(report["native_qualification_required"])

