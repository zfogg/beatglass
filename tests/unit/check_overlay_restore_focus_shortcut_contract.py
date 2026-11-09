"""Verify the keyboard shortcut focuses the restore-overlay action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayRestoreFocusShortcut"')
    block = QML[start:start + 850]
    for marker in ('sequence: "Ctrl+Alt+Shift+W"', "navigation.currentIndex = 1", "restoreOverlay.forceActiveFocus()", "Restore overlay control focused"):
        assert marker in block, marker
    print("Overlay restore focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
