from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class ReadmeQualificationTest(unittest.TestCase):
    def test_readme_links_status_and_disclaims_native_support(self):
        text = (ROOT / "README.md").read_text(encoding="utf-8")
        self.assertIn("docs/platform-qualification-status.md", text)
        self.assertIn("docs/linux-qualification-plan.md", text)
        self.assertIn("does not establish native platform qualification", text)

    def test_native_evidence_template_requires_reviewable_fields(self):
        text = (ROOT / "docs" / "linux-native-evidence-template.md").read_text(encoding="utf-8")
        for heading in ("## Build identity", "## Capture observations", "## Overlay observations", "## Result"):
            self.assertIn(heading, text)
        self.assertIn("Exact configure, build, and test commands", text)
        self.assertIn("Linux support remains experimental and unsupported", text)


if __name__ == "__main__":
    unittest.main()
