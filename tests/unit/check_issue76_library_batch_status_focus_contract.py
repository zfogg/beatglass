"""Contract checks for Library batch status focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryBatchStatus")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "analysisBatchStatus.forceActiveFocus()" in block
    assert "Library analysis batch status focused" in block
    assert "window.focusLibraryBatchStatus()" in QML
if __name__ == "__main__":
    main()
