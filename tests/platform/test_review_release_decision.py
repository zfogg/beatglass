import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("decision", Path(__file__).with_name("review_release_decision.py"))
decision = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(decision)


class ReviewReleaseDecisionTest(unittest.TestCase):
    fixture = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"

    def test_pending_bundle_emits_auditable_pending_decision(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); shutil.copyfile(self.fixture, root / "readiness-report.json")
            packet = root / "review.json"
            review_spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
            review = importlib.util.module_from_spec(review_spec); assert review_spec.loader
            review_spec.loader.exec_module(review)
            packet.write_text(json.dumps(review.review_packet(root / "readiness-report.json")), encoding="utf-8")
            result = decision.decide(root)
            self.assertEqual(result["decision"], "PENDING")
            self.assertTrue(result["reasons"])
            self.assertEqual(len(result["report_sha256"]), 64)

    def test_missing_bundle_cannot_be_accepted(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = decision.decide(Path(tmp))
            self.assertEqual(result["decision"], "PENDING")
            self.assertIsNone(result["report_sha256"])
