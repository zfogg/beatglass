from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class ReadmeQualificationTest(unittest.TestCase):
    def test_readme_links_status_and_disclaims_native_support(self):
        text = (ROOT / "README.md").read_text(encoding="utf-8")
        self.assertIn("docs/platform-qualification-status.md", text)
        self.assertIn("docs/linux-qualification-plan.md", text)
        self.assertIn("does not establish native platform qualification", text)


if __name__ == "__main__":
    unittest.main()
