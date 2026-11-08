import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

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
            self.assertIn("capture", text)
            self.assertIn("Artifacts:", text)
            self.assertIn("## ingestion_errors", text)
