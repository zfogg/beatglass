"""Contract checks for shared Library folder chooser behavior."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function openLibraryFolderChooser")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "folderPicker.open()" in block
    assert "chooseFolder.forceActiveFocus()" in block
    assert "Choose a Library folder dialog opened" in block
    assert "window.openLibraryFolderChooser()" in QML
if __name__ == "__main__":
    main()
