"""Contract checks for folder-picker cancellation."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function handleLibraryFolderChooserRejected")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "Library folder selection canceled" in block
    assert "chooseFolder.forceActiveFocus()" in block
    assert "window.handleLibraryFolderChooserRejected()" in QML
if __name__ == "__main__":
    main()
