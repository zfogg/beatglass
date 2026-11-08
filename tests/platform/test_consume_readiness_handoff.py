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
