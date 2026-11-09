"""Verify the keyboard shortcut focuses saved-settings reapply."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayReapplyFocusShortcut"')
    block = QML[start:start + 850]
    for marker in ('sequence: "Ctrl+Alt+Shift+P"', "navigation.currentIndex = 1", "reapplyOverlaySettings.forceActiveFocus()", "Reapply saved settings control focused"):
        assert marker in block, marker
    print("Overlay reapply focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
