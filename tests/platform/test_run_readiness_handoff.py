import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("handoff", Path(__file__).with_name("run_readiness_handoff.py"))
handoff = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(handoff)


class ReadinessHandoffTest(unittest.TestCase):
    def test_generates_and_validates_conservative_report(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "readiness.json"
            self.assertEqual(handoff.run(bundles, output), [])
            report = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(report["exit_gate_status"], "PENDING")
            self.assertTrue(report["native_qualification_required"])

    def test_status_matrix_check_reports_incomplete_native_evidence(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; status = root / "status.md"
            status.write_text("| Windows | prep | gates |\n| macOS | prep | gates |\n| Linux | prep | gates |\n", encoding="utf-8")
            errors = handoff.run(bundles, output, status, max_age_hours=100000)
            self.assertTrue(any("required scenarios are not PASS" in error for error in errors))
            report = json.loads(output.read_text(encoding="utf-8"))
            self.assertTrue(report["handoff_check_errors"])

    def test_documentation_checker_is_recorded_in_handoff(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "readiness.json"
            self.assertEqual(handoff.run(bundles, output, check_docs=True), [])
            report = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(report["documentation_check_errors"], [])

    def test_consumer_is_connected_and_rejects_pending_output(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "readiness.json"
            errors = handoff.run(bundles, output, consume_report=True)
            self.assertTrue(any("not accepted: PENDING" in error for error in errors))

    def test_native_ingestion_is_connected_to_handoff(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); native = root / "native"; bundles = root / "bundles"; bundles.mkdir()
            artifact = native / "build" / "app.bin"; artifact.parent.mkdir(parents=True); artifact.write_bytes(b"fixture")
            import hashlib
            digest = hashlib.sha256(b"fixture").hexdigest()
            (native / "qualification-artifacts.sha256").write_text(f"{digest}  build/app.bin\n", encoding="utf-8")
            (native / "qualification-manifest.txt").write_text("platform=windows\ncommit=" + "0" * 40 + "\ncaptured_at=2026-11-08T00:00:00Z\nenvironment=test\n", encoding="utf-8")
            output = root / "readiness.json"
            errors = handoff.run(bundles, output, native_output=native, native_platform="windows")
            self.assertFalse(any("missing qualification-manifest" in error for error in errors))
            self.assertEqual(json.loads((bundles / "windows/manifest.json").read_text())["scenarios"][0]["status"], "PENDING")

    def test_tampered_native_output_is_recorded_in_handoff(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); native = root / "native"; bundles = root / "bundles"; bundles.mkdir()
            native.mkdir(); (native / "qualification-artifacts.sha256").write_text("0" * 64 + "  missing.bin\n", encoding="utf-8")
            (native / "qualification-manifest.txt").write_text("platform=windows\n", encoding="utf-8")
            output = root / "readiness.json"
            errors = handoff.run(bundles, output, native_output=native, native_platform="windows")
            self.assertTrue(any("missing or escaping artifact" in error for error in errors))
            report = json.loads(output.read_text(encoding="utf-8"))
            self.assertTrue(report["ingestion_errors"])

    def test_native_platform_identity_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); native = root / "native"; bundles = root / "bundles"; bundles.mkdir(); native.mkdir()
            (native / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (native / "qualification-manifest.txt").write_text("platform=macos\n", encoding="utf-8")
            errors = handoff.run(bundles, root / "readiness.json", native_output=native, native_platform="windows")
            self.assertTrue(any("platform mismatch" in error for error in errors))

    def test_full_handoff_pipeline_review_is_nonzero_and_auditable(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; status = root / "status.md"
            status.write_text("| Windows | prep | gates |\n| macOS | prep | gates |\n| Linux | prep | gates |\n", encoding="utf-8")
            errors = handoff.run(bundles, output, status, 100000, check_docs=True, consume_report=True)
            report = json.loads(output.read_text(encoding="utf-8"))
            self.assertTrue(errors)
            self.assertEqual(report["exit_gate_status"], "PENDING")
            self.assertIn("handoff_check_errors", report)
            self.assertIn("documentation_check_errors", report)
            self.assertIn("ingestion_errors", report)

    def test_handoff_exports_human_review_checklist(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; review = root / "review.md"
            handoff.run(bundles, output, review_output=review)
            self.assertIn("Gate status: **PENDING**", review.read_text(encoding="utf-8"))

    def test_handoff_strict_review_gate_rejects_pending(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; review = root / "review.md"
            errors = handoff.run(bundles, output, review_output=review, require_ready=True)
            self.assertTrue(any("review gate is not ready" in error for error in errors))

    def test_handoff_exports_machine_review_packet(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; packet = root / "review.json"
            handoff.run(bundles, output, review_json=packet)
            data = json.loads(packet.read_text(encoding="utf-8"))
            self.assertEqual(data["gate_status"], "PENDING")
            self.assertEqual(len(data["platforms"]), 3)

    def test_handoff_consumes_emitted_review_packet(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; packet = root / "review.json"
            errors = handoff.run(bundles, output, review_json=packet, consume_review=True)
            self.assertTrue(any("review packet" in error for error in errors))

    def test_strict_gate_automatically_consumes_review_packet(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; packet = root / "review.json"
            errors = handoff.run(bundles, output, review_json=packet, require_ready=True)
            self.assertTrue(any("review packet" in error for error in errors))

    def test_strict_gate_consumes_report_and_packet_as_retained_bundle(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; packet = root / "review.json"
            handoff.run(bundles, output, review_json=packet)
            output.write_text("{}", encoding="utf-8")
            errors = handoff.bundle_consumer.consume(root, output.name, packet.name)
            self.assertTrue(any("source report hash mismatch" in error for error in errors))

    def test_handoff_writes_conservative_release_decision(self):
        bundles = Path(__file__).parent / "fixtures/readiness-bundles"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); output = root / "readiness.json"; packet = root / "review.json"; decision = root / "decision.json"
            errors = handoff.run(bundles, output, review_json=packet, decision_output=decision)
            data = json.loads(decision.read_text(encoding="utf-8"))
            self.assertEqual(data["decision"], "PENDING")
            self.assertTrue(any("release decision is PENDING" in error for error in errors))

