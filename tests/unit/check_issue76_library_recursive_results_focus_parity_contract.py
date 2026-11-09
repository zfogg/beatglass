"""Contract checks for the dedicated recursive-results focus shortcut."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index('objectName: "libraryRecursiveResultsFocusShortcut"')
    block = QML[start:QML.index('objectName: "libraryCatalogStatusFocusShortcut"', start)]
    assert "window.recursiveCatalogActive" in block
    assert "window.focusLibraryResults()" in block
if __name__ == "__main__":
    main()
