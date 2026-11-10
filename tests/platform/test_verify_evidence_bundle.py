import importlib.util
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("verify_bundle", Path(__file__).with_name("verify_evidence_bundle.py"))
module = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(module)
readiness_spec = importlib.util.spec_from_file_location("readiness", Path(__file__).with_name("check_evidence_readiness.py"))
readiness = importlib.util.module_from_spec(readiness_spec); assert readiness_spec.loader
readiness_spec.loader.exec_module(readiness)


class EvidenceBundleTest(unittest.TestCase):
    def test_missing_provenance_and_required_scenario_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); artifact = root / "capture.txt"; artifact.write_text("x\n", encoding="utf-8")
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"artifacts": [{"path": "capture.txt", "sha256": "bad"}], "scenarios": []}), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("missing provenance" in error for error in errors))
            self.assertTrue(any("missing required scenarios" in error for error in errors))

    def test_malformed_manifest_and_artifact_entries_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest = root / "manifest.json"
            manifest.write_text("{not-json", encoding="utf-8")
            self.assertTrue(any("invalid manifest JSON" in error for error in module.verify(root, manifest)))
            manifest.write_text(json.dumps({"artifacts": ["bad"], "scenarios": []}), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("invalid artifact entry" in error for error in errors))

    def test_artifact_hash_must_be_a_string(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": ["bad"], "role": "capture"}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in errors))

    def test_artifact_unknown_fields_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "size": 1}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("unsupported artifact fields: ['size']", module.verify(root, manifest))

    def test_artifact_hash_must_be_a_sha256_digest(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); artifact = root / "capture.txt"; artifact.write_text("x\n", encoding="utf-8")
            manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "not-a-digest", "role": "capture"}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in errors))

    def test_artifact_hash_must_be_lowercase(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); artifact = root / "capture.txt"; artifact.write_text("x\n", encoding="utf-8")
            manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "A" * 64, "role": "capture"}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in errors))

    def test_capture_timestamp_must_be_timezone_aware_iso8601(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "not-a-time",
                    "environment": "test", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertIn("captured_at must be ISO-8601", errors)
            data["captured_at"] = "2020-01-01T00:00:00"
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("captured_at must include timezone", module.verify(root, manifest))

    def test_capture_timestamp_cannot_be_in_the_future(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2999-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("captured_at cannot be in the future", module.verify(root, manifest))

    def test_commit_provenance_must_be_a_sha1(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "main", "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("commit must be a 40-character hexadecimal SHA-1", module.verify(root, manifest))

    def test_platform_provenance_must_be_supported(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "android", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("unsupported platform: android", module.verify(root, manifest))

    def test_artifact_role_must_be_nonempty_string_when_present(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "role": []}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("artifact role must be a nonempty string: capture.txt", module.verify(root, manifest))

    def test_artifact_role_must_be_trimmed_when_present(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "role": " capture "}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("artifact role must not have leading or trailing whitespace: capture.txt", module.verify(root, manifest))

    def test_artifact_role_has_bounded_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "role": "x" * 129}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("artifact role exceeds 128 characters: capture.txt", module.verify(root, manifest))

    def test_artifact_path_with_nul_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\u0000.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_be_trimmed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": " capture.txt ", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_use_forward_slashes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "captures\\capture.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_dot_segments(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "captures/./capture.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_empty_segments(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "captures//capture.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_has_bounded_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "a" * 513, "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_colons(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "captures:report.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_control_characters(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\n.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_del_control_character(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\u007f.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_paths_must_not_collide_case_insensitively(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "Capture.txt", "sha256": "0" * 64}, {"path": "capture.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_segments_must_be_windows_safe(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture. ", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_windows_device_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "CON.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_windows_reserved_characters(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture?.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_components_have_bounded_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "a" * 256, "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_use_nfc_unicode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "cafe\u0301.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_unicode_noncharacters(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\ufdd0.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_bidi_controls(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\u202e.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_format_controls(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\u200b.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_unicode_line_separators(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\u2028.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_unicode_space_separators(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\u00a0.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_surrogate_codepoints(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\ud800.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_replacement_character(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\ufffd.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_private_use_characters(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\ue000.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_artifact_path_must_not_use_unassigned_codepoints(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture\u0378.txt", "sha256": "0" * 64}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, manifest)))

    def test_environment_provenance_rejects_control_characters(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "host\nidentity", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("environment must not contain control characters", module.verify(root, manifest))

    def test_platform_provenance_must_be_lowercase(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "Windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("platform must be lowercase", module.verify(root, manifest))

    def test_environment_provenance_must_be_trimmed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": " host", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("environment must not have leading or trailing whitespace", module.verify(root, manifest))

    def test_environment_provenance_has_bounded_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "x" * 513, "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("environment exceeds 512 characters", module.verify(root, manifest))

    def test_artifact_inventory_has_bounded_size(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            artifacts = [{"path": f"capture-{index}.txt", "sha256": "0" * 64} for index in range(1001)]
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": artifacts,
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("artifacts exceed 1000 entries", module.verify(root, manifest))

    def test_commit_provenance_must_be_lowercase(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "A" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("commit must be a 40-character hexadecimal SHA-1", module.verify(root, manifest))

    def test_scenario_id_must_be_hashable_string(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            scenarios = [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]
            scenarios[0] = {"id": ["capture"], "status": "PENDING"}
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "role": "capture"}],
                    "scenarios": scenarios}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("invalid scenario" in error for error in errors))
            self.assertTrue(any("missing required scenarios" in error for error in errors))

    def test_repeated_structured_scenario_ids_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            scenarios = [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]
            scenarios[0] = {"id": ["malformed"], "status": "PENDING"}
            scenarios[1] = {"id": ["malformed"], "status": "PENDING"}
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "role": "capture"}],
                    "scenarios": scenarios}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertGreaterEqual(sum("invalid scenario" in error for error in errors), 2)

    def test_structured_artifact_path_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": ["capture.txt"], "sha256": "0" * 64, "role": "capture"}],
                    "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in errors))

    def test_structured_scenario_status_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            scenarios = [{"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]
            scenarios[0] = {"id": "capture", "status": ["PENDING"]}
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "role": "capture"}],
                    "scenarios": scenarios}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("invalid scenario" in error for error in errors))

    def test_pending_scenario_is_allowed_without_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [], "scenarios": [
                        {"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertFalse(any("invalid scenario" in error for error in errors))

    def test_incomplete_scenario_cannot_claim_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); evidence = root / "capture.txt"; evidence.write_text("x", encoding="utf-8")
            manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": hashlib.sha256(b"x").hexdigest(), "role": "capture"}],
                    "scenarios": [{"id": scenario, "status": "PENDING", "evidence": "capture.txt"} if scenario == "capture"
                                  else {"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("incomplete scenario must not claim evidence" in error for error in errors))

    def test_completed_scenario_rejects_non_string_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); manifest = root / "manifest.json"
            data = {"platform": "windows", "commit": "0" * 40, "captured_at": "2020-01-01T00:00:00Z",
                    "environment": "test", "artifacts": [{"path": "capture.txt", "sha256": "0" * 64, "role": "capture"}],
                    "scenarios": [{"id": scenario, "status": "PASS", "evidence": ["capture.txt"]} if scenario == "capture"
                                  else {"id": scenario, "status": "PENDING"} for scenario in module.REQUIRED_SCENARIOS]}
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("scenario evidence is not retained" in error for error in errors))

    def test_pass_fixture_is_valid(self):
        root = Path(__file__).parent / "fixtures/evidence-bundle"
        self.assertEqual(module.verify(root, root / "manifest.json"), [])

def manifest(artifact):
    artifact.setdefault("role", "capture")
    return {"platform": "test", "commit": "0" * 40, "captured_at": "2026-11-08T00:00:00Z", "environment": "unit", "artifacts": [artifact], "scenarios": [{"id": "capture", "status": "UNRUN"}]}


class EvidenceBundleNegativeTest(unittest.TestCase):
    def test_completed_scenario_requires_artifact_role(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "capture.txt").write_text("actual\n", encoding="utf-8")
            digest = __import__("hashlib").sha256(b"actual\n").hexdigest()
            data = manifest({"path": "capture.txt", "sha256": digest})
            data["artifacts"][0].pop("role", None)
            data["scenarios"] = [{"id": "capture", "status": "PASS", "evidence": "capture.txt"}]
            path = root / "manifest.json"; path.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("role is missing" in error for error in module.verify(root, path)))

    def test_completed_scenario_requires_matching_artifact_role(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "capture.txt").write_text("actual\n", encoding="utf-8")
            digest = __import__("hashlib").sha256(b"actual\n").hexdigest()
            data = manifest({"path": "capture.txt", "sha256": digest, "role": "output"})
            data["scenarios"] = [{"id": "capture", "status": "PASS", "evidence": "capture.txt"}]
            path = root / "manifest.json"; path.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("role mismatch" in error for error in module.verify(root, path)))

    def test_unknown_and_duplicate_scenarios_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "capture.txt").write_text("actual\n", encoding="utf-8")
            digest = __import__("hashlib").sha256(b"actual\n").hexdigest()
            data = manifest({"path": "capture.txt", "sha256": digest})
            data["scenarios"] = [
                {"id": "capture", "status": "UNRUN"},
                {"id": "capture", "status": "UNRUN"},
                {"id": "overlay-typo", "status": "UNRUN"},
            ]
            path = root / "manifest.json"; path.write_text(json.dumps(data), encoding="utf-8")
            errors = module.verify(root, path)
            self.assertTrue(any("unknown scenarios" in error for error in errors))
            self.assertTrue(any("duplicate scenarios" in error for error in errors))

    def test_artifact_path_alias_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "capture.txt").write_text("actual\n", encoding="utf-8")
            digest = __import__("hashlib").sha256(b"actual\n").hexdigest()
            data = manifest({"path": "capture.txt", "sha256": digest})
            data["artifacts"].append({"path": "./capture.txt", "sha256": digest})
            path = root / "manifest.json"; path.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("invalid or duplicate artifact entry" in error for error in module.verify(root, path)))

    def test_passed_scenarios_cannot_share_one_evidence_artifact(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "evidence.json").write_text("{}", encoding="utf-8")
            data = manifest({"path": "evidence.json", "sha256": __import__("hashlib").sha256(b"{}").hexdigest()})
            data["artifacts"][0]["role"] = "capture"
            data["scenarios"] = [
                {"id": "capture", "status": "PASS", "evidence": "evidence.json"},
                {"id": "recovery", "status": "PASS", "evidence": "evidence.json"},
            ]
            path = root / "manifest.json"; path.write_text(json.dumps(data), encoding="utf-8")
            self.assertTrue(any("scenario evidence is shared" in error for error in module.verify(root, path)))

    def test_hash_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "capture.txt").write_text("actual\n", encoding="utf-8")
            path = root / "manifest.json"; path.write_text(json.dumps(manifest({"path": "capture.txt", "sha256": "0" * 64})), encoding="utf-8")
            self.assertTrue(any("hash mismatch" in error for error in module.verify(root, path)))

    def test_path_escape_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); path = root / "manifest.json"
            path.write_text(json.dumps(manifest({"path": "../outside.txt", "sha256": "0" * 64})), encoding="utf-8")
            self.assertTrue(any("escapes bundle" in error for error in module.verify(root, path)))

    def test_readiness_requires_each_platform_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; bundles.mkdir()
            status = root / "status.md"
            status.write_text("| Windows | prep | gates |\n| macOS | prep | gates |\n| Linux | prep | gates |\n", encoding="utf-8")
            (bundles / "windows").mkdir()
            self.assertIn("macos: missing manifest", "\n".join(readiness.check(status, bundles)))

    def test_readiness_cli_rejects_unrun_required_scenarios(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            (bundles / "windows" / "capture.txt").write_text("actual\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": __import__("hashlib").sha256(b"actual\n").hexdigest()})
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            self.assertIn("required scenarios are not PASS", "\n".join(readiness.check(status, bundles)))

    def test_readiness_rejects_platform_mismatch(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            (bundles / "windows" / "manifest.json").write_text(json.dumps(manifest({"path": "capture.txt", "sha256": "0" * 64})), encoding="utf-8")
            self.assertIn("platform does not match", "\n".join(readiness.check(status, bundles)))

    def test_readiness_report_records_retention_and_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            (bundles / "windows" / "manifest.json").write_text(json.dumps(manifest({"path": "capture.txt", "sha256": "0" * 64})), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertFalse(result["ready"])
            self.assertEqual(result["platforms"][0]["retained_artifacts"], 1)
            self.assertTrue(result["platforms"][0]["errors"])
            self.assertIn("artifact_role_summary", result)
            self.assertEqual(result["artifact_role_summary"][0]["missing"], [])

    def test_readiness_rejects_bad_identity_and_stale_capture(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["commit"] = "not-a-sha"; data["captured_at"] = "2020-01-01T00:00:00Z"
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            errors = "\n".join(readiness.check(status, bundles, max_age_hours=1))
            self.assertIn("invalid source commit", errors)
            self.assertIn("evidence is stale", errors)

    def test_readiness_report_is_reproducible_and_records_retained_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            artifact = bundles / "windows" / "capture.txt"; artifact.write_text("actual\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["artifacts"][0]["sha256"] = __import__("hashlib").sha256(artifact.read_bytes()).hexdigest()
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            first = readiness.report(status, bundles)
            second = readiness.report(status, bundles)
            self.assertEqual(first, second)
            self.assertEqual(first["platforms"][0]["retained_bytes"], artifact.stat().st_size)
            self.assertTrue(first["platforms"][0]["manifest_sha256"])
            self.assertEqual(first["platforms"][0]["artifact_inventory"][0]["path"], "capture.txt")
            self.assertEqual(first["platforms"][0]["artifact_inventory"][0]["bytes"], artifact.stat().st_size)

    def test_readiness_rejects_duplicate_scenarios_and_reports_statuses(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["scenarios"] = [{"id": "capture", "status": "UNRUN"}, {"id": "capture", "status": "PASS"}]
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertIn("duplicate scenario", "\n".join(result["platforms"][0]["errors"]))
            self.assertEqual([item["status"] for item in result["platforms"][0]["scenario_inventory"]], ["UNRUN", "PASS"])

    def test_readiness_rejects_unlisted_retained_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            bundle = bundles / "windows"
            (bundle / "extra.log").write_text("not declared", encoding="utf-8")
            (bundle / "manifest.json").write_text(json.dumps(manifest({"path": "capture.txt", "sha256": "0" * 64})), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertIn("unlisted retained files", "\n".join(result["platforms"][0]["errors"]))

    def test_readiness_rejects_duplicate_status_platform_row(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; bundles.mkdir()
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n| Windows | prep | gates |\n", encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertFalse(result["ready"])
            self.assertIn("duplicate platform rows", "\n".join(result["platforms"][0]["errors"]))

    def test_readiness_report_propagates_failure_summary(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; bundles.mkdir()
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertFalse(result["ready"])
            self.assertEqual(result["error_count"], len(result["errors"]))
            self.assertTrue(any("missing manifest" in error for error in result["errors"]))

    def test_readiness_reports_missing_artifact_inventory(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "missing.json", "sha256": "0" * 64})
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platforms"][0]["missing_artifacts"], ["missing.json"])
            self.assertTrue(result["platforms"][0]["invalidated"])

    def test_readiness_reports_hash_mismatch_inventory(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            (bundles / "windows" / "capture.txt").write_text("actual", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platforms"][0]["mismatched_artifacts"], ["capture.txt"])
            self.assertEqual(result["platforms"][0]["manifest_platform"], "test")
            self.assertTrue(result["platforms"][0]["provenance_complete"])

    def test_readiness_reports_missing_provenance_fields(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64}); data.pop("environment")
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platforms"][0]["missing_provenance"], ["environment"])

    def test_readiness_reports_missing_scenarios(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platforms"][0]["missing_scenarios"], ["display", "output", "overlay", "playback", "recovery"])

    def test_readiness_reports_scenario_status_counts(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["scenarios"] = [{"id": "capture", "status": "PASS"}, {"id": "recovery", "status": "UNRUN"}]
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platforms"][0]["scenario_status_counts"], {"PASS": 1, "UNRUN": 1})
            self.assertIn("recovery", result["platforms"][0]["unpassed_scenarios"])

    def test_readiness_reports_freshness_details(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64}); data["captured_at"] = "2020-01-01T00:00:00Z"
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles, max_age_hours=1)
            self.assertTrue(result["platforms"][0]["freshness_errors"])

    def test_readiness_reports_invalid_scenarios(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64}); data["scenarios"] = [{"id": "capture", "status": "BOGUS"}]
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platforms"][0]["invalid_scenarios"], ["capture"])

    def test_readiness_malformed_scenario_inventory_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["scenarios"] = [{"id": ["capture"], "status": ["PASS"]}]
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertTrue(result["platforms"][0]["invalid_scenarios"])

    def test_readiness_malformed_artifact_path_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": ["capture.txt"], "sha256": "0" * 64})
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertTrue(result["platforms"][0]["errors"])

    def test_readiness_malformed_evidence_reference_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["scenarios"] = [{"id": "capture", "status": "PASS", "evidence": ["capture.txt"]}]
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertIn("capture", result["platforms"][0]["missing_artifact_roles"])

    def test_readiness_non_list_manifest_containers_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["artifacts"] = {"path": "capture.txt"}
            data["scenarios"] = {"id": "capture", "status": "PASS"}
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertTrue(result["platforms"][0]["errors"])

    def test_readiness_check_malformed_scenario_id_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["scenarios"] = [{"id": ["capture"], "status": "PASS"}]
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            errors = readiness.check(status, bundles)
            self.assertTrue(any("invalid scenario" in error for error in errors))

    def test_readiness_non_object_manifest_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            (bundles / "windows" / "manifest.json").write_text("[\"malformed\"]", encoding="utf-8")
            self.assertTrue(any("manifest root must be an object" in error for error in readiness.check(status, bundles)))
            result = readiness.report(status, bundles)
            self.assertTrue(any("manifest root must be an object" in error for error in result["platforms"][0]["errors"]))

    def test_readiness_mixed_artifact_paths_have_stable_diagnostics(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["artifacts"].append({"path": ["malformed.txt"], "sha256": "0" * 64, "role": "bad"})
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertTrue(result["platforms"][0]["missing_artifacts"])

    def test_readiness_check_null_manifest_containers_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "capture.txt", "sha256": "0" * 64})
            data["artifacts"] = None; data["scenarios"] = None
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            errors = readiness.check(status, bundles)
            self.assertTrue(any("artifacts must be a nonempty list" in error for error in errors))

    def test_readiness_aggregates_required_scenario_summary(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; bundles.mkdir()
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["required_scenario_summary"][0]["platform"], "windows")
            self.assertIn("capture", result["required_scenario_summary"][0]["missing"])

    def test_readiness_aggregates_provenance_summary(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; bundles.mkdir()
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertFalse(result["provenance_summary"][0]["complete"])
            self.assertIn("platform", result["provenance_summary"][0]["missing"])
            self.assertTrue(result["summary_consistent"])

    def test_readiness_reports_platform_summary_consistency(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; bundles.mkdir()
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platform_count"], 1)
            self.assertFalse(result["platform_summary_consistent"])

    def test_readiness_reports_platform_failure_summary(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; bundles.mkdir()
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            result = readiness.report(status, bundles)
            self.assertEqual(result["platform_failure_summary"][0]["platform"], "windows")
            self.assertTrue(result["platform_failure_summary"][0]["invalidated"])
            self.assertFalse(result["platform_failure_summary"][0]["ready"])
            self.assertEqual(result["ready_platform_count"], 0)
            self.assertEqual(result["retained_artifact_count"], 0)
            self.assertEqual(result["retained_artifact_bytes"], 0)
            self.assertEqual(result["retained_artifacts_by_platform"], {"windows": 0})
            self.assertEqual(result["retained_bytes_by_platform"], {"windows": 0})
            self.assertTrue(result["retained_bytes_consistent"])
            self.assertTrue(result["inventory_bytes_consistent"])
            self.assertTrue(result["inventory_count_consistent"])
            self.assertFalse(result["all_platforms_ready"])
            self.assertEqual(result["exit_gate_status"], "PENDING")
            self.assertTrue(result["exit_gate_reasons"])
            self.assertIn("manifest", result["exit_gate_reason_codes"])
            self.assertIn("native_qualification_summary", result)
            self.assertFalse(result["native_qualification_ready"])
            self.assertTrue(result["native_summary_consistent"])
            self.assertTrue(result["native_qualification_blockers"])
            self.assertEqual(result["native_qualification_blockers"][0]["platform"], "windows")
            self.assertTrue(result["native_blockers_consistent"])
            self.assertEqual(result["platform_support_status"][0]["status"], "PENDING")
            self.assertIn("scenario", result["platform_support_status"][0]["reason_codes"])
            self.assertTrue(result["platform_support_status_consistent"])
            self.assertEqual(result["support_language"], "experimental")
            self.assertTrue(result["native_qualification_required"])
            self.assertEqual(result["readiness_report_schema"], 1)
            self.assertEqual(result["not_ready_platforms"], ["windows"])
            self.assertTrue(result["failure_summary_consistent"])
            self.assertFalse(result["aggregate_consistent"])
            self.assertTrue(result["error_count_consistent"])
            self.assertTrue(result["platforms"][0]["failure_category_count_consistent"])
            self.assertTrue(result["failure_category_count_consistent"])
            self.assertIn("retention", result["failure_category_platforms"])
            self.assertTrue(result["category_platforms_consistent"])
            self.assertTrue(result["category_platform_lists_consistent"])
            self.assertFalse(result["category_counts_consistent"])

    def test_readiness_aggregates_failure_categories(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundles = root / "bundles"; (bundles / "windows").mkdir(parents=True)
            status = root / "status.md"; status.write_text("| Windows | prep | gates |\n", encoding="utf-8")
            data = manifest({"path": "missing.json", "sha256": "0" * 64})
            data["captured_at"] = "2020-01-01T00:00:00Z"
            (bundles / "windows" / "manifest.json").write_text(json.dumps(data), encoding="utf-8")
            result = readiness.report(status, bundles, max_age_hours=1)
            self.assertGreater(result["failure_categories"]["retention"], 0)
            self.assertGreater(result["failure_categories"]["freshness"], 0)
            self.assertGreater(result["platforms"][0]["failure_categories"]["freshness"], 0)
            self.assertFalse(result["platforms"][0]["retention_ok"])
            self.assertEqual(result["platforms"][0]["error_count"], len(result["platforms"][0]["errors"]))


if __name__ == "__main__":
    unittest.main()
