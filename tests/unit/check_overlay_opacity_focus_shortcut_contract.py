"""Verify the keyboard shortcut focuses the overlay opacity slider."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayOpacityFocusShortcut"')
    block = QML[start:start + 900]
    for marker in ('sequence: "Ctrl+Alt+L"', "navigation.currentIndex = 1", "overlayOpacityControl.forceActiveFocus()", "current value is"):
        assert marker in block, marker
    print("Overlay opacity focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
