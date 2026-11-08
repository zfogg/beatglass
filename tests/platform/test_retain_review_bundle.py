import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("retainer", Path(__file__).with_name("retain_review_bundle.py"))
retainer = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(retainer)


class ReviewBundleRetentionTest(unittest.TestCase):
    report_fixture = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"

    def _packet(self, root: Path) -> Path:
        review_spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
        review = importlib.util.module_from_spec(review_spec); assert review_spec.loader
        review_spec.loader.exec_module(review)
        packet = root / "review.json"
        packet.write_text(json.dumps(review.review_packet(self.report_fixture)), encoding="utf-8")
        return packet

    def test_retains_and_verifies_bound_pair(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); packet = self._packet(root)
            destination = root / "retained"
            errors = retainer.retain(self.report_fixture, packet, destination)
            self.assertTrue((destination / "readiness-report.json").is_file())
            self.assertTrue((destination / "review.json").is_file())
            # The fixture is intentionally pending, so export remains diagnostic.
            self.assertTrue(any("acceptance errors" in e or "not accepted" in e for e in errors))

    def test_missing_source_is_rejected_without_creating_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); destination = root / "retained"
            errors = retainer.retain(root / "missing.json", root / "packet.json", destination)
            self.assertIn("missing source report", errors[0])
            self.assertFalse(destination.exists())

    def test_detached_packet_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); packet = self._packet(root)
            packet.write_text("{}", encoding="utf-8")
            errors = retainer.retain(self.report_fixture, packet, root / "retained")
            self.assertTrue(any("retained review bundle rejected" in e for e in errors))
