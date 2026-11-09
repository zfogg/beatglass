"""Verify the display-recovery shortcut focuses the overlay selector."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayDisplayFocusShortcut"')
    block = QML[start:start + 900]
    for marker in ('sequence: "Ctrl+Alt+D"', "navigation.currentIndex = 1", "overlayDisplay.forceActiveFocus()", "Overlay display selection required"):
        assert marker in block, marker
    print("Overlay display focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
