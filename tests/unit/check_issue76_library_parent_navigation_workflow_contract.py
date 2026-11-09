"""Contract checks for shared parent-folder navigation behavior."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function navigateLibraryParentFolder")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "window.browseFolder = files.parentFolder" in block
    assert 'search.text = ""' in block
    assert "recursiveCatalogRows = []" in block
    assert "resetLibraryBrowserSelection" in block
    assert "window.navigateLibraryParentFolder()" in QML
if __name__ == "__main__":
    main()
