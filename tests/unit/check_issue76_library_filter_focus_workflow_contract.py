"""Contract checks for the Library filter focus shortcut."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryFilter")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "search.forceActiveFocus()" in block
    assert "search.selectAll()" in block
    assert "Library filter focused" in block
    assert "window.focusLibraryFilter()" in QML
if __name__ == "__main__":
    main()
