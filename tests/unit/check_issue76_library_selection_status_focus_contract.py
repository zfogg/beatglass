"""Contract checks for Library selection-status focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibrarySelectionStatus")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "librarySelectionStatus.forceActiveFocus()" in block
    assert "Library selection status focused" in block
    assert "window.focusLibrarySelectionStatus()" in QML
if __name__ == "__main__":
    main()
