"""Verify the keyboard shortcut focuses the show-overlay action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayShowFocusShortcut"')
    block = QML[start:start + 800]
    for marker in ('sequence: "Ctrl+Alt+W"', "navigation.currentIndex = 1", "showOverlay.forceActiveFocus()", "Show overlay control focused"):
        assert marker in block, marker
    print("Overlay show focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
