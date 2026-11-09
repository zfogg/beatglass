"""Contract checks that refresh makes its selection-reset behavior explicit."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index('objectName: "refreshFolder"')
    block = QML[start:QML.index('objectName: "folderPath"', start)]
    assert 'Accessible.value: enabled ? "available; clears the current selection"' in block
    assert "window.clearSelectedAudio()" in block
    assert "browser.currentIndex = -1" in block
    assert "refreshCatalog()" in block
if __name__ == "__main__":
    main()
