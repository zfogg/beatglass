"""Verify Ctrl+Alt+Backspace clears the selected Library source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"libraryClearShortcut\"", "sequence: \"Ctrl+Alt+Backspace\"", "window.clearSelectedAudio(); search.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 Library clear shortcut contract verified (3 checks)")
if __name__ == "__main__": main()
