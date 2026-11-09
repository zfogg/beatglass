import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("bundle", Path(__file__).with_name("consume_review_bundle.py"))
bundle = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(bundle)


class ReviewBundleConsumerTest(unittest.TestCase):
    fixture_report = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"

    def _write_bundle(self, root: Path) -> None:
        report = root / "readiness-report.json"
        shutil.copyfile(self.fixture_report, report)
        review_spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
        review = importlib.util.module_from_spec(review_spec); assert review_spec.loader
        review_spec.loader.exec_module(review)
        (root / "review.json").write_text(json.dumps(review.review_packet(report)), encoding="utf-8")

    def test_bundle_requires_both_retained_artifacts(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); self._write_bundle(root)
            errors = bundle.consume(root)
            self.assertTrue(any("acceptance errors" in error or "not accepted" in error for error in errors))
            (root / "review.json").unlink()
            self.assertIn("missing retained review packet: review.json", bundle.consume(root))

    def test_bundle_rejects_report_replacement(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); self._write_bundle(root)
            (root / "readiness-report.json").write_text("{}", encoding="utf-8")
            self.assertIn("unsupported readiness_report_schema", bundle.consume(root))
            self.assertIn("review packet source report hash mismatch", bundle.consume(root))
