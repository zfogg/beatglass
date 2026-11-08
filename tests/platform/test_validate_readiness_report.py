import importlib.util
import json
import shutil
import tempfile
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("validate_report", Path(__file__).with_name("validate_readiness_report.py"))
module = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(module)


class ReadinessReportContractTest(unittest.TestCase):
    def test_incomplete_fixture_has_valid_conservative_contract(self):
        report = __import__("json").loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        self.assertEqual(module.validate(report), [])

    def test_ready_report_cannot_hide_reason_codes(self):
        report = {"readiness_report_schema": 1, "support_language": "experimental", "native_qualification_required": True,
                  "exit_gate_status": "READY", "exit_gate_reason_codes": ["scenario"],
                  "platform_support_status": [{"platform": p} for p in ("windows", "macos", "linux")]}
        self.assertIn("READY report cannot contain", "\n".join(module.validate(report)))

    def test_report_rejects_non_object_platform_rows(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"].append("windows")
        report["platform_support_status"].append("linux")
        errors = chr(10).join(module.validate(report))
        self.assertIn("platforms must contain exactly", errors)
        self.assertIn("platform_support_status must contain exactly", errors)

    def test_report_requires_platform_manifest_and_provenance_linkage(self):
        report = __import__("json").loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0].pop("manifest_sha256")
        report["platforms"][1]["provenance_complete"] = False
        errors = "\n".join(module.validate(report))
        self.assertIn("invalid manifest hash linkage", errors)
        self.assertIn("provenance is incomplete", errors)

    def test_report_rejects_invalid_artifact_inventory(self):
        report = __import__("json").loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["artifact_inventory"][0]["hash_valid"] = False
        self.assertIn("artifact hash is not valid", "\n".join(module.validate(report)))

    def test_retained_files_match_manifest_and_report(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = __import__("json").loads((root / "readiness-report.json").read_text(encoding="utf-8"))
        self.assertEqual(module.validate_retained_files(report, root), [])

    def test_retained_file_hash_mismatch_is_rejected(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = __import__("json").loads((root / "readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["artifact_inventory"][0]["sha256"] = "0" * 64
        self.assertIn("hash mismatch", "\n".join(module.validate_retained_files(report, root)))

    def test_qualification_output_must_be_retained(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = __import__("json").loads((root / "readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["qualification_output"] = "missing-output.txt"
        errors = "\n".join(module.validate_retained_files(report, root))
        self.assertIn("qualification output does not match", errors)

    def test_qualification_output_hash_is_verified(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = __import__("json").loads((root / "readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["qualification_output_sha256"] = "0" * 64
        errors = "\n".join(module.validate_retained_files(report, root))
        self.assertIn("output hash does not match", errors)

    def test_qualification_output_bytes_are_verified(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = __import__("json").loads((root / "readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["qualification_output_bytes"] = 0
        errors = "\n".join(module.validate_retained_files(report, root))
        self.assertIn("output byte count does not match", errors)

    def test_scenario_evidence_hash_is_verified(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = __import__("json").loads((root / "readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["scenario_evidence_sha256"]["capture"] = "0" * 64
        errors = "\n".join(module.validate_retained_files(report, root))
        self.assertIn("scenario evidence hash does not match", errors)

    def test_scenario_evidence_hashes_must_be_object(self):
        root = Path(__file__).parent / "fixtures/readiness-bundles"
        report = __import__("json").loads((root / "readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["scenario_evidence_sha256"] = []
        errors = "\n".join(module.validate_retained_files(report, root))
        self.assertIn("scenario evidence hashes must be an object", errors)

    def test_missing_scenario_evidence_is_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["scenarios"][0]["evidence"] = "missing-evidence.json"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("retained scenario evidence is missing", errors)

    def test_scenario_evidence_cannot_escape_bundle(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["scenarios"][0]["evidence"] = "../manifest.json"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("scenario evidence escapes retained bundle", errors)

    def test_invalid_manifest_json_is_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            (root / "windows" / "manifest.json").write_text("{not-json", encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("retained manifest is invalid JSON", errors)

    def test_manifest_collection_shapes_are_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["artifacts"] = {}
            manifest["scenarios"] = {}
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("manifest artifacts must be an array", errors)
            self.assertIn("manifest scenarios must be an array", errors)
