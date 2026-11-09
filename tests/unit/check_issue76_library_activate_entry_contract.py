"""Contract checks for Library focused-entry activation."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function activateLibraryFocusedEntry")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "window.activateEntry(browser.currentIndex)" in block
    assert "browser.forceActiveFocus()" in block
    assert "window.activateLibraryFocusedEntry()" in QML
if __name__ == "__main__":
    main()
