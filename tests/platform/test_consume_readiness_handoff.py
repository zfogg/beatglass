import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("consumer", Path(__file__).with_name("consume_readiness_handoff.py"))
consumer = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(consumer)


class ReadinessConsumerTest(unittest.TestCase):
    def test_pending_fixture_is_not_accepted(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        self.assertTrue(any("not accepted: PENDING" in error for error in consumer.consume(source)))

    def test_invalid_report_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "report.json"
            report.write_text(json.dumps({"exit_gate_status": "READY"}), encoding="utf-8")
            self.assertTrue(consumer.consume(report))

    def test_ready_report_with_ingestion_error_is_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        report = json.loads(source.read_text(encoding="utf-8"))
        report["exit_gate_status"] = "READY"
        report["ingestion_errors"] = ["hash mismatch: app.bin"]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "report.json"; path.write_text(json.dumps(report), encoding="utf-8")
            self.assertTrue(any("contains ingestion_errors" in error for error in consumer.consume(path)))

    def test_ready_report_requiring_native_qualification_is_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        report = json.loads(source.read_text(encoding="utf-8"))
        report.update({"exit_gate_status": "READY", "ready": True, "exit_gate_reason_codes": [],
                       "handoff_check_errors": [], "documentation_check_errors": [], "ingestion_errors": [],
                       "generation_errors": []})
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "report.json"; path.write_text(json.dumps(report), encoding="utf-8")
            self.assertTrue(any("native qualification is required" in error for error in consumer.consume(path)))
