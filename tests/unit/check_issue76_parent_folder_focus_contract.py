"""Verify parent-folder navigation announces and restores browser focus."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("window.browseFolder = files.parentFolder", "Moved to the parent Library folder", "browser.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 parent folder focus contract verified (3 checks)")
if __name__ == "__main__": main()
