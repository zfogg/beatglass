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
