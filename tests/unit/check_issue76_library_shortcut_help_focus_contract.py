"""Contract checks for Library shortcut-help focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index("function focusLibraryShortcutHelp")
    block = QML[start:QML.index("function browserEmptyMessage", start)]
    assert "navigation.currentIndex = 0" in block
    assert "libraryShortcutHelp.forceActiveFocus()" in block
    assert "Library keyboard shortcut help focused" in block
    assert "window.focusLibraryShortcutHelp()" in QML
if __name__ == "__main__":
    main()
