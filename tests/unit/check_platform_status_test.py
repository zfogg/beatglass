from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class PlatformStatusDocumentationTest(unittest.TestCase):
    def test_matrix_preserves_native_qualification_gates(self):
        text = (ROOT / "docs" / "platform-qualification-status.md").read_text(encoding="utf-8")
        for platform in ("Windows", "macOS", "Linux"):
            self.assertIn(f"| {platform} |", text)
        self.assertIn("does not establish native desktop, audio, or overlay support", text)
        self.assertIn("Issue #76 UI gate", text)
        self.assertIn("main controls stay recoverable", text)


if __name__ == "__main__":
    unittest.main()
