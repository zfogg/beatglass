"""Contract checks that keyboard refresh shares the browser reset path."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index('objectName: "libraryRefreshShortcut"')
    block = QML[start:QML.index('objectName: "libraryParentFolderShortcut"', start)]
    assert "files.refresh()" in block
    assert "window.refreshCatalog()" in block
    assert "window.resetLibraryBrowserSelection" in block
if __name__ == "__main__":
    main()
