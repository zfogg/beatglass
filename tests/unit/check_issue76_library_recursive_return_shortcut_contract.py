"""Contract checks for recursive return shortcut behavior."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function returnLibraryFromRecursiveShortcut")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "window.returnFromRecursiveResults()" in block
    assert "window.returnLibraryFromRecursiveShortcut()" in QML
if __name__ == "__main__":
    main()
