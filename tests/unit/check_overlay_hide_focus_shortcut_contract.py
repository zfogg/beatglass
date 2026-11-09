"""Verify the keyboard shortcut focuses the hide-overlay action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayHideFocusShortcut"')
    block = QML[start:start + 700]
    for marker in ('sequence: "Ctrl+Alt+J"', "navigation.currentIndex = 1", "hideOverlay.forceActiveFocus()", "Hide overlay control focused"):
        assert marker in block, marker
    print("Overlay hide focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
