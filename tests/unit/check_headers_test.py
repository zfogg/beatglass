"""Regression coverage for the production header inventory."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("check_headers", Path(__file__).resolve().parents[2] / "docs/api/check_headers.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class HeaderCoverageTest(unittest.TestCase):
    def test_new_private_lib_header_requires_index_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "docs/api").mkdir(parents=True)
            (root / "lib/library").mkdir(parents=True)
            (root / "lib/library/private.hpp").write_text("#pragma once\n", encoding="utf-8")
            index = root / "docs/api/index.rst"
            index.write_text("API\n===\n", encoding="utf-8")
            self.assertEqual(module.coverage(root), (["lib/library/private.hpp"], []))
            index.write_text(".. cpp:autodoc:: lib/library/private.hpp\n", encoding="utf-8")
            self.assertEqual(module.coverage(root), ([], []))


if __name__ == "__main__":
    unittest.main()
