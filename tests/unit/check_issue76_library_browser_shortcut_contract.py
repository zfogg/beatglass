"""Verify Ctrl+Alt+L focuses the Library browser."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"libraryBrowserShortcut\"", "sequence: \"Ctrl+Alt+L\"", "browser.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 Library browser shortcut contract verified (3 checks)")
if __name__ == "__main__": main()
