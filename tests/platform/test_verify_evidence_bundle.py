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


if __name__ == "__main__":
    unittest.main()
