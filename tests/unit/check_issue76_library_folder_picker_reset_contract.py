"""Contract checks that choosing a folder resets the shared browser state."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("id: folderPicker")
    block = QML[start:QML.index("onRejected:", start)]
    assert "window.resetLibraryBrowserSelection" in block
    assert "window.recursiveCatalogActive = false" in block
    assert "window.recursiveCatalogRows = []" in block
    assert "search.text = \"\"" in block
if __name__ == "__main__":
    main()
