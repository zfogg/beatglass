import importlib.util
from pathlib import Path
import tempfile
import unittest
import hashlib
import json

spec = importlib.util.spec_from_file_location("ingest", Path(__file__).with_name("ingest_qualification_artifacts.py"))
ingest = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(ingest)


class QualificationIngestTest(unittest.TestCase):
    def test_ingests_hashed_native_output_as_pending_manifest(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); artifact = root / "build" / "app.bin"; artifact.parent.mkdir(); artifact.write_bytes(b"fixture")
            digest = hashlib.sha256(b"fixture").hexdigest()
            (root / "qualification-artifacts.sha256").write_text(f"{digest}  build/app.bin\n", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text("platform=windows\ncommit=" + "0" * 40 + "\ncaptured_at=2026-11-08T00:00:00Z\nenvironment=test\n", encoding="utf-8")
            output = root / "manifest.json"
            self.assertEqual(ingest.ingest(root, output), [])
            data = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(data["artifacts"][0]["path"], "build/app.bin")
            self.assertTrue(all(item["status"] == "PENDING" for item in data["scenarios"]))

    def test_rejects_tampered_retained_artifact(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); artifact = root / "build" / "app.bin"; artifact.parent.mkdir(); artifact.write_bytes(b"changed")
            (root / "qualification-artifacts.sha256").write_text("0" * 64 + "  build/app.bin\n", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text("platform=windows\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("hash mismatch" in error for error in errors))

    def test_rejects_duplicate_artifact_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); artifact = root / "build" / "app.bin"; artifact.parent.mkdir(); artifact.write_bytes(b"fixture")
            digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text(f"{digest}  build/app.bin\n{digest}  ./build/app.bin\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("duplicate artifact path" in error for error in errors))

    def test_rejects_escaping_artifact_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "qualification-artifacts.sha256").write_text("0" * 64 + "  ../escape.bin\n", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text("platform=windows\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("missing or escaping artifact" in error for error in errors))

    def test_rejects_invalid_identity_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text("platform=windows\ncommit=unknown\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("invalid source commit identity", errors)
            self.assertIn("missing capture time", errors)

    def test_rejects_invalid_capture_timestamp(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text("platform=windows\ncommit=" + "0" * 40 + "\ncaptured_at=not-a-time\n", encoding="utf-8")
            self.assertIn("invalid capture timestamp", ingest.ingest(root, root / "manifest.json"))

    def test_ingests_referenced_native_evidence_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "native-playback.json").write_text("{}", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "platform=windows\ncommit=" + "0" * 40 + "\ncaptured_at=2026-11-08T00:00:00Z\n"
                "playback_evidence=native-playback.json\n", encoding="utf-8")
            self.assertEqual(ingest.ingest(root, root / "manifest.json"), [])
            data = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(data["artifacts"][0]["role"], "playback_evidence")

    def test_rejects_referenced_evidence_path_already_hashed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            evidence = root / "native.json"
            evidence.write_text("{}", encoding="utf-8")
            digest = hashlib.sha256(evidence.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(
                f"{digest}  native.json\n", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n"
                "playback_evidence=native.json\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("duplicate referenced evidence path: native.json", errors)

    def test_rejects_invalid_artifact_hash_before_reading_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            artifact = root / "native.log"
            artifact.write_text("native output", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text(
                "not-a-sha256  native.log\n", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("invalid artifact hash: native.log", errors)

    def test_sorts_artifacts_for_reproducible_reports(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            first = root / "z-last.log"; second = root / "a-first.log"
            first.write_bytes(b"z"); second.write_bytes(b"a")
            first_hash = hashlib.sha256(first.read_bytes()).hexdigest()
            second_hash = hashlib.sha256(second.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(
                f"{first_hash}  z-last.log\n{second_hash}  a-first.log\n", encoding="utf-8")
            output = root / "manifest.json"
            self.assertEqual(ingest.ingest(root, output), [])
            data = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual([item["path"] for item in data["artifacts"]], ["a-first.log", "z-last.log"])

    def test_rejects_symlinked_referenced_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            target = root / "outside.json"
            target.write_text("{}", encoding="utf-8")
            link = root / "playback.json"
            try:
                link.symlink_to(target)
            except (OSError, NotImplementedError):
                self.skipTest("symlinks unavailable")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n"
                "playback_evidence=playback.json\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("symlinked referenced evidence" in error for error in errors))

    def test_rejects_empty_qualification_artifact(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            empty = root / "empty.log"; empty.write_bytes(b"")
            digest = hashlib.sha256(empty.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(f"{digest}  empty.log\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("empty qualification artifact" in error for error in errors))

    def test_rejects_symlinked_qualification_artifact(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            target = root / "outside.log"
            target.write_bytes(b"native evidence")
            link = root / "linked.log"
            try:
                link.symlink_to(target)
            except (OSError, NotImplementedError):
                self.skipTest("symlinks unavailable")
            digest = hashlib.sha256(target.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(
                f"{digest}  linked.log\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("symlinked qualification artifact" in error for error in errors))
