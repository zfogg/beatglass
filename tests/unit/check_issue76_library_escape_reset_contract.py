"""Contract checks for the Library browser Escape reset workflow."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function clearLibrarySelectionAndFilter")
    block = QML[start:QML.index("function focusLibraryFilter", start)]
    assert "window.clearSelectedAudio()" in block
    assert 'search.text = ""' in block
    assert "browser.currentIndex = -1" in block
    assert "target.forceActiveFocus()" in block
    assert "window.clearLibrarySelectionAndFilter(search)" in QML
if __name__ == "__main__":
    main()
