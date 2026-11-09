"""Verify the keyboard shortcut focuses the click-through control."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayClickThroughFocusShortcut"')
    block = QML[start:start + 950]
    for marker in ('sequence: "Ctrl+Alt+K"', "navigation.currentIndex = 1", "clickThroughToggle.forceActiveFocus()", "current mode is"):
        assert marker in block, marker
    print("Overlay click-through focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
