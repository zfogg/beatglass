"""Contract checks for recursive catalog action focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryRecursiveAction")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "addFolderRecursively.forceActiveFocus()" in block
    assert "Library recursive catalog action focused" in block
    assert "window.focusLibraryRecursiveAction()" in QML
if __name__ == "__main__":
    main()
