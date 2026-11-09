"""Contract checks that parent navigation resets browser state."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index('objectName: "parentFolder"')
    block = QML[start:QML.index('objectName: "refreshFolder"', start)]
    assert "window.browseFolder = files.parentFolder" in block
    assert "window.resetLibraryBrowserSelection" in block
    assert "window.recursiveCatalogActive = false" in block
    assert "window.recursiveCatalogRows = []" in block
if __name__ == "__main__":
    main()
