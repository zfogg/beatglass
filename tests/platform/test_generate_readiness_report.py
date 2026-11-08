import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

path = Path(__file__).with_name("generate_readiness_report.py")
spec = importlib.util.spec_from_file_location("generate", path)
module = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(module)


class GenerateReadinessReportTest(unittest.TestCase):
    def test_generation_is_conservative_and_reproducible(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = module.build_report(root)
        self.assertEqual(report["exit_gate_status"], "PENDING")
        self.assertFalse(report["ready"])
        self.assertTrue(report["native_qualification_required"])
        self.assertEqual({r["platform"] for r in report["platforms"]}, set(module.PLATFORMS))
        self.assertTrue(all(a["hash_valid"] for r in report["platforms"] for a in r["artifact_inventory"]))

    def test_missing_artifact_remains_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for platform in module.PLATFORMS:
                bundle = root / platform; bundle.mkdir()
                (bundle / "capture.txt").write_text("evidence", encoding="utf-8")
                manifest = {"platform": platform, "commit": "x", "captured_at": "now", "environment": "test",
                            "artifacts": [{"path": "missing.bin", "sha256": "0" * 64}], "scenarios": []}
                (bundle / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            report = module.build_report(root)
            self.assertFalse(report["ready"])
            self.assertTrue(any(not a["hash_valid"] for r in report["platforms"] for a in r["artifact_inventory"]))
            self.assertTrue(any("missing artifact" in error for r in report["platforms"] for error in r["errors"]))

    def test_generation_records_path_safety_errors(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for platform in module.PLATFORMS:
                bundle = root / platform; bundle.mkdir()
                manifest = {"platform": platform, "commit": "x", "captured_at": "now", "environment": "test",
                            "artifacts": [{"path": "../escape.bin", "sha256": "0" * 64}],
                            "qualification_output": "../output.txt",
                            "scenarios": [{"id": "capture", "status": "PASS", "evidence": "../evidence.txt"}]}
                (bundle / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            report = module.build_report(root)
            errors = [error for row in report["platforms"] for error in row["errors"]]
            self.assertTrue(any("escapes bundle" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
