"""Verify the Library folder chooser keyboard shortcut."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'objectName: "libraryChooseFolderShortcut"' in QML
    assert 'sequence: "Ctrl+Alt+F"' in QML
    assert 'navigation.currentIndex = 0; folderPicker.open()' in QML
    print("Issue #76 choose-folder shortcut contract verified")
if __name__ == "__main__": main()
