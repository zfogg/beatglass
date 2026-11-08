import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("verify_bundle", Path(__file__).with_name("verify_evidence_bundle.py"))
module = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(module)


class EvidenceBundleTest(unittest.TestCase):
    def test_missing_provenance_and_required_scenario_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); artifact = root / "capture.txt"; artifact.write_text("x\n", encoding="utf-8")
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"artifacts": [{"path": "capture.txt", "sha256": "bad"}], "scenarios": []}), encoding="utf-8")
            errors = module.verify(root, manifest)
            self.assertTrue(any("missing provenance" in error for error in errors))
            self.assertTrue(any("missing required scenarios" in error for error in errors))

    def test_pass_fixture_is_valid(self):
        root = Path(__file__).parent / "fixtures/evidence-bundle"
        self.assertEqual(module.verify(root, root / "manifest.json"), [])

def manifest(artifact):
    return {"platform": "test", "commit": "0" * 40, "captured_at": "2026-11-08T00:00:00Z", "environment": "unit", "artifacts": [artifact], "scenarios": [{"id": "capture", "status": "UNRUN"}]}


class EvidenceBundleNegativeTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
