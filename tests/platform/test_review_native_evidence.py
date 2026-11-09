import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_native_evidence.py"))
review = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(review)


class NativeEvidenceReviewTest(unittest.TestCase):
    def test_fixture_remains_pending_with_unrun_native_scenarios(self):
        report = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        result = review.assess(report, "release-owner")
        self.assertEqual(result["decision"], "PENDING")
        self.assertTrue(any("recovery" in blocker for blocker in result["blockers"]))

    def test_missing_reviewer_is_blocker(self):
        report = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        result = review.assess(report, " ")
        self.assertIn("reviewer identity is required", result["blockers"])

    def test_pass_cannot_reference_missing_or_escaping_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            report = root / "report.json"
            scenarios = [{"id": scenario, "status": "PASS", "evidence": "missing.json"}
                         for scenario in review.REQUIRED]
            report.write_text('{"platforms":[{"platform":"Linux","scenarios":' + __import__("json").dumps(scenarios) + '}]}')
            result = review.assess(report, "release-owner")
            self.assertEqual(result["decision"], "PENDING")
            self.assertTrue(result["blockers"])

    def test_identity_and_artifact_consistency_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text('{"platforms":[{"platform":"linux","manifest_platform":"windows",'
                              '"commit":"not-a-commit","provenance_complete":false,'
                              '"retained_artifacts":2,"artifact_inventory":[],"scenarios":[]}]}')
            result = review.assess(report, "release-owner")
            self.assertEqual(result["decision"], "PENDING")
            self.assertTrue(any("identity" in blocker for blocker in result["blockers"]))
            self.assertTrue(any("artifact count" in blocker for blocker in result["blockers"]))
