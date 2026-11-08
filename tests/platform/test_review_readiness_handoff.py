import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
review = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(review)


class ReadinessReviewTest(unittest.TestCase):
    def test_review_checklist_preserves_pending_scenarios_and_roles(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "report.json"; output = Path(tmp) / "review.md"
            report.write_text(source.read_text(encoding="utf-8"), encoding="utf-8")
            output.write_text(review.render(report), encoding="utf-8")
            text = output.read_text(encoding="utf-8")
            self.assertIn("Gate status: **PENDING**", text)
            self.assertIn("Native qualification required: **True**", text)
            self.assertIn("## Platform support", text)
            self.assertIn("Manifest SHA-256:", text)
            self.assertIn("Source commit:", text)
            self.assertIn("Qualification command:", text)
            self.assertIn("Qualification output:", text)
            self.assertIn("capture", text)
            self.assertIn("Artifacts:", text)
            self.assertIn("## ingestion_errors", text)
            self.assertIn("## Acceptance diagnostics", text)
            self.assertIn("readiness handoff is not accepted: PENDING", text)

    def test_strict_review_gate_rejects_pending_report(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "review.md"
            with mock.patch("sys.argv", ["review_readiness_handoff.py", str(source), str(output), "--require-ready"]):
                self.assertEqual(review.main(), 1)

    def test_strict_review_gate_runs_consumer_validation(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "review.md"
            report = json.loads(source.read_text(encoding="utf-8")); report["exit_gate_status"] = "READY"
            report_path = Path(tmp) / "report.json"; report_path.write_text(json.dumps(report), encoding="utf-8")
            with mock.patch("sys.argv", ["review_readiness_handoff.py", str(report_path), str(output), "--require-ready"]):
                self.assertEqual(review.main(), 1)

    def test_machine_review_packet_preserves_gate_and_diagnostics(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        packet = review.review_packet(source)
        self.assertEqual(packet["review_packet_schema"], 1)
        self.assertEqual(len(packet["report_sha256"]), 64)
        self.assertEqual(packet["gate_status"], "PENDING")
        self.assertTrue(packet["acceptance_errors"])
        self.assertEqual(len(packet["platforms"]), 3)
        self.assertEqual(review.validate_packet(packet), [])

    def test_review_packet_rejects_unknown_schema(self):
        self.assertIn("unsupported review_packet_schema", review.validate_packet({"review_packet_schema": 99}))
