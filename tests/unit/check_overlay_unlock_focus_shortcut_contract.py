"""Verify the keyboard shortcut focuses the unlock-overlay action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayUnlockFocusShortcut"')
    block = QML[start:start + 800]
    for marker in ('sequence: "Ctrl+Alt+U"', "clickThrough", "unlockOverlay.forceActiveFocus()", "press Enter to restore interaction"):
        assert marker in block, marker
    print("Overlay unlock focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
