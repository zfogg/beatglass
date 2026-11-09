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

    def test_artifact_hash_and_size_provenance_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            scenarios = [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "retained_artifacts": 1,
                                      "artifact_inventory": [{"path": "capture.txt", "sha256": "0" * 64,
                                                               "hash_valid": False, "bytes": 0}],
                                      "scenarios": scenarios}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:invalid artifact provenance", result["blockers"])

    def test_manifest_identity_and_capture_command_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "bad", "qualification_command": "",
                                      "qualification_output": "", "retained_artifacts": 0,
                                      "artifact_inventory": [], "scenarios": []}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:invalid manifest identity", result["blockers"])
            self.assertIn("linux:missing qualification command", result["blockers"])

    def test_qualification_output_must_match_retained_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64,
                                      "qualification_command": "native-runner",
                                      "qualification_output": "capture.txt",
                                      "qualification_output_sha256": "1" * 64,
                                      "retained_artifacts": 1,
                                      "artifact_inventory": [{"path": "capture.txt", "sha256": "2" * 64,
                                                               "hash_valid": True, "bytes": 4}],
                                      "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:qualification output linkage mismatch", result["blockers"])

    def test_duplicate_platform_and_scenario_entries_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            platform = {"platform": "linux", "manifest_platform": "linux", "commit": "0" * 40,
                        "provenance_complete": True, "manifest_sha256": "0" * 64,
                        "qualification_command": "runner", "qualification_output": "capture.txt",
                        "retained_artifacts": 0, "artifact_inventory": [],
                        "scenarios": [{"id": "capture", "status": "UNRUN"}, {"id": "capture", "status": "UNRUN"}]}
            report.write_text(__import__("json").dumps({"platforms": [platform, dict(platform)]}))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:duplicate platform entry", result["blockers"])
            self.assertIn("linux:duplicate scenario id", result["blockers"])

    def test_unknown_status_and_nonpass_evidence_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            scenarios = [{"id": "capture", "status": "MAYBE", "evidence": "capture.txt"}]
            scenarios.extend({"id": scenario, "status": "UNRUN"} for scenario in sorted(review.REQUIRED - {"capture"}))
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                      "qualification_output": "capture.txt", "retained_artifacts": 0,
                                      "artifact_inventory": [], "scenarios": scenarios}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:capture:invalid scenario status", result["blockers"])
            self.assertIn("linux:capture:unexpected evidence for non-pass status", result["blockers"])

    def test_report_level_platform_and_gate_consistency_is_required(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"ready": True, "exit_gate_status": "PENDING",
                "platforms": [{"platform": "linux", "scenarios": []}],
                "platform_support_status": [{"platform": "windows", "status": "experimental"}]}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:platform identity mismatch", result["blockers"])
            self.assertIn("report:ready status contradicts exit gate", result["blockers"])

    def test_platform_support_statuses_must_be_unique_and_known(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"platforms": [{"platform": "linux", "scenarios": []}],
                "platform_support_status": [{"platform": "linux", "status": "mystery"},
                                             {"platform": "linux", "status": "experimental"}]}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:duplicate platform support status", result["blockers"])
            self.assertIn("report:invalid platform support status", result["blockers"])

    def test_qualified_status_must_match_scenario_inventory(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            scenarios = [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]
            payload = {"platforms": [{"platform": "linux", "scenarios": scenarios}],
                       "platform_support_status": [{"platform": "linux", "status": "qualified"}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:qualified status without all scenarios passing", result["blockers"])

    def test_retained_byte_count_must_match_artifacts(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                      "qualification_output": "capture.txt", "retained_artifacts": 1,
                                      "retained_bytes": 99,
                                      "artifact_inventory": [{"path": "capture.txt", "sha256": "0" * 64,
                                                               "hash_valid": True, "bytes": 4}],
                                      "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:retained byte count mismatch", result["blockers"])

    def test_scenario_evidence_hashes_must_match_passes(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                      "qualification_output": "capture.txt", "retained_artifacts": 0,
                                      "artifact_inventory": [], "scenario_evidence_sha256": {"capture": "bad"},
                                      "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:capture:invalid scenario evidence hash", result["blockers"])

    def test_duplicate_artifact_paths_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                      "qualification_output": "capture.txt", "retained_artifacts": 2,
                                      "artifact_inventory": [{"path": "capture.txt", "sha256": "0" * 64, "hash_valid": True, "bytes": 1},
                                                              {"path": "capture.txt", "sha256": "1" * 64, "hash_valid": True, "bytes": 1}],
                                      "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:duplicate artifact path", result["blockers"])

    def test_artifact_roles_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                      "qualification_output": "capture.txt", "retained_artifacts": 1,
                                      "artifact_inventory": [{"path": "capture.txt", "role": "", "sha256": "0" * 64,
                                                               "hash_valid": True, "bytes": 1}],
                                      "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:invalid artifact provenance", result["blockers"])

    def test_environment_and_capture_identity_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                      "qualification_output": "capture.txt", "environment": "",
                                      "captured_at": "", "retained_artifacts": 0,
                                      "artifact_inventory": [],
                                      "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:missing environment identity", result["blockers"])
            self.assertIn("linux:missing capture timestamp", result["blockers"])

    def test_capture_timestamp_requires_timezone(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "manifest_platform": "linux",
                                      "commit": "0" * 40, "provenance_complete": True,
                                      "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                      "qualification_output": "capture.txt", "environment": "host",
                                      "captured_at": "2026-11-09T12:00:00", "retained_artifacts": 0,
                                      "artifact_inventory": [],
                                      "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:missing capture timestamp", result["blockers"])

    def test_platform_build_commit_must_match_report_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"commit": "1" * 40,
                       "platforms": [{"platform": "linux", "manifest_platform": "linux",
                                       "commit": "2" * 40, "provenance_complete": True,
                                       "manifest_sha256": "0" * 64, "qualification_command": "runner",
                                       "qualification_output": "capture.txt", "environment": "host",
                                       "captured_at": "2026-11-09T12:00:00Z", "retained_artifacts": 0,
                                       "artifact_inventory": [],
                                       "scenarios": [{"id": scenario, "status": "UNRUN"} for scenario in review.REQUIRED]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:build commit differs from report", result["blockers"])

    def test_scenario_and_inventory_views_must_agree(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            payload = {"platforms": [{"platform": "linux", "scenarios": [],
                                      "scenario_inventory": [{"id": "capture", "status": "UNRUN"}]}]}
            report.write_text(__import__("json").dumps(payload))
            result = review.assess(report, "release-owner")
            self.assertIn("linux:scenario inventory mismatch", result["blockers"])

    def test_unknown_platform_identity_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"platforms": [{"platform": "haiku", "scenarios": []}]}))
            result = review.assess(report, "release-owner")
            self.assertIn("haiku:unknown platform identity", result["blockers"])

    def test_all_three_platforms_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"platforms": [{"platform": "linux", "scenarios": []}]}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:platform inventory incomplete", result["blockers"])

    def test_report_schema_and_native_gate_are_consistent(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"readiness_report_schema": 99,
                "native_qualification_required": False, "ready": True, "platforms": []}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:unsupported readiness schema", result["blockers"])
            self.assertIn("report:ready without native qualification requirement", result["blockers"])

    def test_exit_gate_reason_codes_match_ready_state(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"ready": True, "exit_gate_status": "ACCEPTED",
                "exit_gate_reason_codes": ["stale_blocker"], "platforms": []}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:ready report has exit gate blockers", result["blockers"])

    def test_ready_report_cannot_contain_handoff_errors(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"ready": True,
                "exit_gate_status": "ACCEPTED", "handoff_check_errors": ["missing manifest"],
                "platforms": []}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:ready report has handoff errors", result["blockers"])

    def test_ready_report_cannot_contain_documentation_errors(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"ready": True,
                "exit_gate_status": "ACCEPTED", "documentation_check_errors": ["stale docs"],
                "platforms": []}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:ready report has documentation errors", result["blockers"])

    def test_handoff_status_matches_ready_state(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"ready": True,
                "handoff_status": "pending", "platforms": []}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:ready status contradicts handoff status", result["blockers"])

    def test_accepted_gate_requires_ready_status(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"ready": False,
                "exit_gate_status": "ACCEPTED", "platforms": []}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:accepted gate without ready status", result["blockers"])

    def test_gate_boolean_fields_are_typed(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(__import__("json").dumps({"ready": "true",
                "native_qualification_required": "yes", "platforms": []}))
            result = review.assess(report, "release-owner")
            self.assertIn("report:ready must be boolean", result["blockers"])
            self.assertIn("report:native qualification requirement must be boolean", result["blockers"])
