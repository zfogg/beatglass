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
            self.assertTrue(any("duplicate artifact path" in error or "non-canonical artifact path" in error for error in errors))

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

    def test_rejects_empty_platform_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "platform=\ncommit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            self.assertIn("empty platform identity", ingest.ingest(root, root / "manifest.json"))

    def test_rejects_empty_environment_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "platform=linux\nenvironment=\ncommit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            self.assertIn("empty environment identity", ingest.ingest(root, root / "manifest.json"))

    def test_rejects_invalid_capture_timestamp(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text("platform=windows\ncommit=" + "0" * 40 + "\ncaptured_at=not-a-time\n", encoding="utf-8")
            self.assertIn("invalid capture timestamp", ingest.ingest(root, root / "manifest.json"))

    def test_rejects_timezone_less_capture_timestamp(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "platform=linux\ncommit=" + "0" * 40 + "\ncaptured_at=2026-01-01T00:00:00\n", encoding="utf-8")
            self.assertIn("capture timestamp must include timezone", ingest.ingest(root, root / "manifest.json"))

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

    def test_rejects_empty_referenced_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "playback.json").write_bytes(b"")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n"
                "playback_evidence=playback.json\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("empty referenced evidence: playback.json", errors)

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

    def test_rejects_noncanonical_artifact_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            artifact = root / "native.log"
            artifact.write_text("native output", encoding="utf-8")
            digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(
                f"{digest}  a/../native.log\n", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("non-canonical artifact path: a/../native.log", errors)

    def test_rejects_symlinked_qualification_metadata(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            target_manifest = root / "manifest-source.txt"
            target_manifest.write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            target_hashes = root / "hashes-source.txt"
            target_hashes.write_text("", encoding="utf-8")
            try:
                (root / "qualification-manifest.txt").symlink_to(target_manifest)
                (root / "qualification-artifacts.sha256").symlink_to(target_hashes)
            except (OSError, NotImplementedError):
                self.skipTest("symlinks unavailable")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("symlinked qualification-manifest.txt", errors)
            self.assertIn("symlinked qualification-artifacts.sha256", errors)

    def test_rejects_symlinked_manifest_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            target = root / "target.json"
            target.write_text("sentinel", encoding="utf-8")
            output = root / "manifest.json"
            try:
                output.symlink_to(target)
            except (OSError, NotImplementedError):
                self.skipTest("symlinks unavailable")
            errors = ingest.ingest(root, output)
            self.assertIn("symlinked evidence manifest output", errors)
            self.assertEqual(target.read_text(encoding="utf-8"), "sentinel")

    def test_rejects_directory_manifest_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text(
                "commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            output = root / "manifest.json"
            output.mkdir()
            errors = ingest.ingest(root, output)
            self.assertIn("evidence manifest output is not a file", errors)

    def test_rejects_missing_manifest_output_parent(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            errors = ingest.ingest(root, root / "missing" / "manifest.json")
            self.assertIn("evidence manifest output parent is missing", errors)

    def test_rejects_symlinked_manifest_output_parent(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            target = root / "target"
            target.mkdir()
            parent = root / "linked-output"
            try:
                parent.symlink_to(target, target_is_directory=True)
            except (OSError, NotImplementedError):
                self.skipTest("symlinks unavailable")
            errors = ingest.ingest(root, parent / "manifest.json")
            self.assertIn("symlinked evidence manifest output parent", errors)

    def test_rejects_symlinked_qualification_root(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            real_root = base / "real"
            real_root.mkdir()
            (real_root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (real_root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            root = base / "linked"
            try:
                root.symlink_to(real_root, target_is_directory=True)
            except (OSError, NotImplementedError):
                self.skipTest("symlinks unavailable")
            errors = ingest.ingest(root, base / "manifest.json")
            self.assertIn("symlinked qualification root", errors)

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

    def test_nonportable_referenced_evidence_path_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\nplayback_evidence=capture?.json\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("non-portable referenced evidence path: capture?.json", errors)

    def test_referenced_evidence_uses_artifact_size_limit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            evidence = root / "playback.json"; evidence.write_text("large", encoding="utf-8")
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\nplayback_evidence=playback.json\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json", max_artifact_bytes=1)
            self.assertIn("referenced evidence exceeds 1 bytes: playback.json", errors)

    def test_total_artifact_size_limit_is_enforced(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            artifact = root / "capture.log"; artifact.write_text("large", encoding="utf-8")
            digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(f"{digest}  capture.log\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json", max_total_artifact_bytes=1)
            self.assertIn("qualification artifacts exceed total 1 bytes", errors)

    def test_custom_artifact_limit_is_enforced(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            artifact = root / "capture.log"; artifact.write_text("native evidence", encoding="utf-8")
            digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(f"{digest}  capture.log\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json", max_artifact_bytes=1)
            self.assertTrue(any("qualification artifact exceeds 1 bytes" in error for error in errors))

    def test_invalid_resource_limits_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(["resource limits must be positive"], ingest.ingest(root, root / "manifest.json", max_entries=0))

    def test_uppercase_artifact_hash_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            artifact = root / "capture.log"; artifact.write_text("native evidence", encoding="utf-8")
            digest = hashlib.sha256(artifact.read_bytes()).hexdigest().upper()
            (root / "qualification-artifacts.sha256").write_text(f"{digest}  capture.log\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("invalid artifact hash" in error for error in errors))

    def test_nonportable_artifact_path_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text(f"{'0' * 64}  capture?.log\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("non-portable artifact path" in error for error in errors))

    def test_identity_provenance_is_canonicalized(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "A" * 40 + "\nplatform=Windows\nenvironment= host\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("source commit identity must be lowercase", errors)
            self.assertIn("invalid platform identity", errors)
            self.assertIn("invalid environment identity", errors)

    def test_future_capture_timestamp_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            future = (ingest.datetime.now(ingest.timezone.utc) + ingest.timedelta(hours=1)).isoformat()
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=" + future + "\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn("capture timestamp cannot be in the future", errors)

    def test_capture_timestamp_is_emitted_as_canonical_utc(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T01:00:00-05:00\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            self.assertEqual(ingest.ingest(root, root / "manifest.json"), [])
            data = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(data["captured_at"], "2026-01-01T06:00:00Z")

    def test_generated_manifest_is_written_atomically_without_temp_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("", encoding="utf-8")
            output = root / "manifest.json"
            self.assertEqual(ingest.ingest(root, output), [])
            self.assertTrue(output.is_file())
            self.assertEqual(list(root.glob(".manifest.json.*.tmp")), [])

    def test_oversized_identity_input_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("x" * (ingest.MAX_IDENTITY_BYTES + 1), encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn(f"qualification identity exceeds {ingest.MAX_IDENTITY_BYTES} bytes", errors)

    def test_oversized_hash_input_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            (root / "qualification-artifacts.sha256").write_text("x" * (ingest.MAX_HASHES_BYTES + 1), encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertIn(f"qualification hash list exceeds {ingest.MAX_HASHES_BYTES} bytes", errors)

    def test_case_colliding_artifact_paths_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "qualification-manifest.txt").write_text("commit=" + "a" * 40 + "\ncaptured_at=2026-01-01T00:00:00Z\n", encoding="utf-8")
            first = root / "Capture.log"; second = root / "capture.log"
            first.write_text("one", encoding="utf-8"); second.write_text("two", encoding="utf-8")
            digest = hashlib.sha256(first.read_bytes()).hexdigest()
            digest2 = hashlib.sha256(second.read_bytes()).hexdigest()
            (root / "qualification-artifacts.sha256").write_text(f"{digest}  Capture.log\n{digest2}  capture.log\n", encoding="utf-8")
            errors = ingest.ingest(root, root / "manifest.json")
            self.assertTrue(any("duplicate artifact path" in error or "non-canonical artifact path" in error for error in errors))
