import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("promoter", Path(__file__).with_name("promote_review_bundle.py"))
promoter = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(promoter)


class ReviewBundlePromotionTest(unittest.TestCase):
    def test_promotion_code_marks_native_status_pending(self):
        source = (Path(__file__).with_name("promote_review_bundle.py")).read_text(encoding="utf-8")
        self.assertIn('"native_qualification_status": "pending"', source)

    def test_accepted_promotion_writes_pending_native_status(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "source"; destination = Path(tmp) / "release"; source.mkdir()
            (source / "readiness-report.json").write_text("report", encoding="utf-8")
            (source / "review.json").write_text("packet", encoding="utf-8")
            with patch.object(promoter.decision, "decide", return_value={"decision": "ACCEPTED", "reasons": []}):
                accepted, errors = promoter.promote(source, destination)
            self.assertTrue(accepted, errors)
            manifest = json.loads((destination / "release-manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["native_qualification_status"], "pending")

    def test_pending_bundle_cannot_be_promoted(self):
        fixture = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "source"; destination = Path(tmp) / "release"
            source.mkdir()
            shutil.copyfile(fixture / "readiness-report.json", source / "readiness-report.json")
            review_spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
            review = importlib.util.module_from_spec(review_spec); assert review_spec.loader
            review_spec.loader.exec_module(review)
            (source / "review.json").write_text(__import__("json").dumps(review.review_packet(source / "readiness-report.json")), encoding="utf-8")
            accepted, errors = promoter.promote(source, destination)
            self.assertFalse(accepted)
            self.assertTrue(errors)
            self.assertFalse(destination.exists())

    def test_pending_bundle_does_not_write_release_manifest(self):
        fixture = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "source"; destination = Path(tmp) / "release"
            source.mkdir()
            shutil.copyfile(fixture / "readiness-report.json", source / "readiness-report.json")
            accepted, _ = promoter.promote(source, destination)
            self.assertFalse(accepted)
            self.assertFalse((destination / "release-manifest.json").exists())

    def test_missing_bundle_cannot_be_promoted(self):
        with tempfile.TemporaryDirectory() as tmp:
            accepted, errors = promoter.promote(Path(tmp) / "missing", Path(tmp) / "release")
            self.assertFalse(accepted)
            self.assertTrue(errors)
