import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

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
        self.assertTrue(all(row["commit"] and row["captured_at"] and row["environment"] for row in report["platforms"]))

    def test_cli_creates_nested_output_directory(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "nested" / "reports" / "readiness.json"
            with mock.patch("sys.argv", ["generate_readiness_report.py", str(root), str(output)]):
                self.assertEqual(module.main(), 0)
            self.assertTrue(output.is_file())
            self.assertEqual(json.loads(output.read_text(encoding="utf-8"))["exit_gate_status"], "PENDING")
            self.assertEqual(list(output.parent.glob(".readiness.json.*.tmp")), [])

    def test_cli_rejects_report_over_size_limit(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "readiness.json"
            with mock.patch("sys.argv", ["generate_readiness_report.py", str(root), str(output), "--max-report-bytes", "1"]):
                self.assertEqual(module.main(), 1)
            self.assertFalse(output.exists())

    def test_cli_json_emits_generated_report(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as temp, mock.patch("sys.argv", ["generate_readiness_report.py", str(root), str(Path(temp) / "report.json"), "--json"]):
            with mock.patch("builtins.print") as printer:
                self.assertEqual(module.main(), 0)
                self.assertTrue(printer.called)

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

    def test_generation_preserves_artifact_roles(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for platform in module.PLATFORMS:
                bundle = root / platform; bundle.mkdir()
                artifact = bundle / "topology.json"; artifact.write_text("{}", encoding="utf-8")
                manifest = {"platform": platform, "commit": "x", "captured_at": "now", "environment": "test",
                            "artifacts": [{"path": "topology.json", "sha256": __import__("hashlib").sha256(b"{}").hexdigest(), "role": "display_topology"}],
                            "scenarios": []}
                (bundle / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            report = module.build_report(root)
            self.assertEqual(report["platforms"][0]["artifact_inventory"][0]["role"], "display_topology")

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

    def test_generation_surfaces_malformed_or_missing_manifests(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for platform in module.PLATFORMS:
                bundle = root / platform
                bundle.mkdir()
                (bundle / "manifest.json").write_text("{not-json", encoding="utf-8")
            report = module.build_report(root)
            self.assertTrue(all(any("invalid manifest JSON" in error for error in row["errors"]) for row in report["platforms"]))
            self.assertEqual(len(report["generation_errors"]), len(module.PLATFORMS))

    def test_generation_surfaces_malformed_manifest_collections(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for platform in module.PLATFORMS:
                bundle = root / platform; bundle.mkdir()
                manifest = {"platform": platform, "artifacts": {}, "scenarios": {}, "qualification_output": 7}
                (bundle / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            report = module.build_report(root)
            errors = [error for row in report["platforms"] for error in row["errors"]]
            self.assertIn("manifest artifacts must be an array", errors)
            self.assertIn("manifest scenarios must be an array", errors)
            self.assertIn("qualification output must be a string", errors)


if __name__ == "__main__":
    unittest.main()
