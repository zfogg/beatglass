"""Contract checks for Library folder chooser focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryFolderChooser")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "chooseFolder.forceActiveFocus()" in block
    assert "Library folder chooser focused" in block
    assert "window.focusLibraryFolderChooser()" in QML
if __name__ == "__main__":
    main()
