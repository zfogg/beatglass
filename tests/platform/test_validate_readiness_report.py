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

    def test_report_rejects_duplicate_platform_rows(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"].append(dict(report["platforms"][0]))
        report["platform_support_status"].append(dict(report["platform_support_status"][0]))
        errors = chr(10).join(module.validate(report))
        self.assertIn("platforms must contain exactly", errors)
        self.assertIn("platform_support_status must contain exactly", errors)

    def test_report_rejects_non_experimental_platform_status(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["platform_support_status"][0]["status"] = "ready"
        errors = chr(10).join(module.validate(report))
        self.assertIn("platform_support_status rows must remain experimental", errors)

    def test_report_root_must_be_object(self):
        self.assertEqual(module.validate([]), ["readiness report must be a JSON object"])
        self.assertEqual(module.validate_retained_files([], Path(".")), ["readiness report must be a JSON object"])

    def test_report_rejects_malformed_reason_codes(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["exit_gate_reason_codes"] = ["", 7]
        errors = chr(10).join(module.validate(report))
        self.assertIn("exit_gate_reason_codes must be a list", errors)

    def test_report_rejects_duplicate_reason_codes(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["exit_gate_reason_codes"] = ["native_qualification_required", "native_qualification_required"]
        errors = chr(10).join(module.validate(report))
        self.assertIn("exit_gate_reason_codes must be a list", errors)

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

    def test_report_rejects_non_hex_digests(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["manifest_sha256"] = "z" * 64
        errors = chr(10).join(module.validate(report))
        self.assertIn("invalid manifest hash linkage", errors)

    def test_report_rejects_duplicate_artifact_paths(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        inventory = report["platforms"][0]["artifact_inventory"]
        inventory.append(dict(inventory[0]))
        errors = chr(10).join(module.validate(report))
        self.assertIn("duplicate report artifact path", errors)

    def test_report_rejects_non_object_artifact_entries(self):
        report = json.loads((Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json").read_text(encoding="utf-8"))
        report["platforms"][0]["artifact_inventory"].append("not-an-artifact")
        errors = chr(10).join(module.validate(report))
        self.assertIn("artifact entry must be an object", errors)

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

    def test_duplicate_manifest_scenario_ids_are_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["scenarios"].append(dict(manifest["scenarios"][0]))
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("duplicate manifest scenario id", errors)

    def test_missing_manifest_scenario_id_is_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["scenarios"][0]["id"] = None
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("manifest scenario id is missing", errors)

    def test_non_string_manifest_platform_is_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["platform"] = 1
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("manifest platform does not match report row", errors)

    def test_manifest_provenance_fields_are_well_formed(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["commit"] = "not-a-commit"
            manifest["captured_at"] = "not-a-time"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("manifest commit must be a 40-character hexadecimal SHA-1", errors)
            self.assertIn("manifest captured_at must be ISO-8601", errors)

    def test_manifest_artifacts_require_roles(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["artifacts"][0].pop("role")
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("manifest artifact role is missing", errors)

    def test_invalid_manifest_scenario_status_is_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["scenarios"][0]["status"] = "MAYBE"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("invalid manifest scenario status", errors)

    def test_scenario_status_and_evidence_must_agree(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["scenarios"][0]["status"] = "PASS"
            manifest["scenarios"][0].pop("evidence")
            manifest["scenarios"][1]["evidence"] = "capture.txt"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("completed scenario is missing evidence", errors)
            self.assertIn("incomplete scenario must not claim evidence", errors)

    def test_scenario_evidence_must_be_manifest_artifact(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["scenarios"][0]["evidence"] = "manifest.json"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("scenario evidence is not listed as an artifact", errors)

    def test_report_inventory_must_match_manifest(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            report["platforms"][0]["artifact_inventory"] = report["platforms"][0]["artifact_inventory"][:-1]
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("report artifact inventory does not match manifest", errors)

    def test_scenario_hash_mapping_must_match_manifest(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            report["platforms"][0]["scenario_evidence_sha256"]["unexpected"] = "0" * 64
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("scenario evidence hash mapping does not match manifest scenarios", errors)

    def test_duplicate_manifest_artifacts_are_rejected(self):
        source = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "bundles"
            shutil.copytree(source, root)
            manifest_path = root / "windows" / "manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["artifacts"].append(dict(manifest["artifacts"][0]))
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = json.loads((root / "readiness-report.json").read_text(encoding="utf-8"))
            errors = chr(10).join(module.validate_retained_files(report, root))
            self.assertIn("duplicate manifest artifact path", errors)
