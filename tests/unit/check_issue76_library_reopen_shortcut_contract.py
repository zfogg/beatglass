"""Verify Ctrl+Alt+Shift+P reopens the selected Library source."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"libraryReopenShortcut\"", "sequence: \"Ctrl+Alt+Shift+P\"", "!window.playbackMatchesSelection", "reopenSelectedPlayback.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 Library reopen shortcut contract verified (4 checks)")
if __name__ == "__main__": main()
