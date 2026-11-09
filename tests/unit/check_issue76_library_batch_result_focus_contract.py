"""Contract checks for Library batch result focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryBatchResult")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "analysisBatchLastResult.forceActiveFocus()" in block
    assert "Library analysis batch result focused" in block
    assert "window.focusLibraryBatchResult()" in QML
if __name__ == "__main__":
    main()
