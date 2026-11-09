import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("bundle", Path(__file__).with_name("verify_release_bundle.py"))
bundle = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(bundle)


class CompleteReleaseBundleTest(unittest.TestCase):
    def test_missing_signoff_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertIn("missing retained release signoff", bundle.verify(Path(tmp))[0])

    def test_release_manifest_requires_signoff(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "signoff.json").write_text("{}", encoding="utf-8")
            errors = bundle.verify(root)
            self.assertTrue(any("invalid release manifest" in error for error in errors))
