"""Verify the keyboard shortcut focuses the overlay size selector."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlaySizeFocusShortcut"')
    block = QML[start:start + 900]
    for marker in ('sequence: "Ctrl+Alt+Z"', "navigation.currentIndex = 1", "overlaySize.forceActiveFocus()", "current size is"):
        assert marker in block, marker
    print("Overlay size focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
