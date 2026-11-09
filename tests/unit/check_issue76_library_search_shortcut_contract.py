"""Verify Ctrl+F focuses and selects the Library filter."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"librarySearchShortcut\"", "sequence: \"Ctrl+F\"", "search.forceActiveFocus()", "search.selectAll()"):
        assert marker in QML, marker
    print("Issue #76 Library search shortcut contract verified (4 checks)")
if __name__ == "__main__": main()
