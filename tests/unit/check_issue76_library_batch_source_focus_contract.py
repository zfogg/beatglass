"""Contract checks for Library batch source focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryBatchSource")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "analysisBatchLastSource.forceActiveFocus()" in block
    assert "Library analysis batch source focused" in block
    assert "window.focusLibraryBatchSource()" in QML
if __name__ == "__main__":
    main()
