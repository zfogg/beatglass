"""Contract checks for the Library clear-selection shortcut."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index('objectName: "libraryClearShortcut"')
    block = QML[start:QML.index('objectName: "libraryCatalogStatusFocusShortcut"', start)]
    assert "window.resetLibraryBrowserSelection" in block
    assert "selectedAudioUrl" in block
if __name__ == "__main__":
    main()
