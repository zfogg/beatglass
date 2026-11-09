import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("verifier", Path(__file__).with_name("verify_release_manifest.py"))
verifier = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(verifier)


class ReleaseManifestVerifierTest(unittest.TestCase):
    def test_missing_manifest_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertTrue(verifier.verify(Path(tmp)))

    def test_tampered_promoted_artifact_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); report = root / "readiness-report.json"; packet = root / "review.json"
            report.write_text("report", encoding="utf-8"); packet.write_text("packet", encoding="utf-8")
            import hashlib
            manifest = {"release_manifest_schema": 1, "decision": "ACCEPTED",
                        "report_sha256": hashlib.sha256(report.read_bytes()).hexdigest(),
                        "review_packet_sha256": hashlib.sha256(packet.read_bytes()).hexdigest()}
            (root / "release-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            self.assertEqual(verifier.verify(root), [])
            report.write_text("tampered", encoding="utf-8")
            self.assertIn("release manifest hash mismatch: readiness-report.json", verifier.verify(root))

    def test_mismatched_signoff_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); report = root / "readiness-report.json"; packet = root / "review.json"
            report.write_text("report", encoding="utf-8"); packet.write_text("packet", encoding="utf-8")
            import hashlib
            manifest = {"release_manifest_schema": 1, "decision": "ACCEPTED",
                        "report_sha256": hashlib.sha256(report.read_bytes()).hexdigest(),
                        "review_packet_sha256": hashlib.sha256(packet.read_bytes()).hexdigest()}
            (root / "release-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            signoff = root / "signoff.json"
            signoff.write_text(json.dumps({"release_signoff_schema": 1, "decision": "ACCEPTED", "reviewer": "owner", "report_sha256": "b" * 64}), encoding="utf-8")
            self.assertIn("release signoff report hash does not match manifest", verifier.verify(root, signoff))

    def test_invalid_native_qualification_status_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); report = root / "readiness-report.json"; packet = root / "review.json"
            report.write_text("report", encoding="utf-8"); packet.write_text("packet", encoding="utf-8")
            import hashlib
            manifest = {"release_manifest_schema": 1, "decision": "ACCEPTED", "native_qualification_status": "claimed",
                        "report_sha256": hashlib.sha256(report.read_bytes()).hexdigest(),
                        "review_packet_sha256": hashlib.sha256(packet.read_bytes()).hexdigest()}
            (root / "release-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            self.assertIn("invalid native qualification status", verifier.verify(root))

    def test_qualified_manifest_requires_native_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); report = root / "readiness-report.json"; packet = root / "review.json"
            report.write_text("report", encoding="utf-8"); packet.write_text("packet", encoding="utf-8")
            import hashlib
            manifest = {"release_manifest_schema": 1, "decision": "ACCEPTED", "native_qualification_status": "qualified",
                        "report_sha256": hashlib.sha256(report.read_bytes()).hexdigest(),
                        "review_packet_sha256": hashlib.sha256(packet.read_bytes()).hexdigest()}
            (root / "release-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            self.assertIn("qualified release manifest lacks native qualification provenance", verifier.verify(root))
