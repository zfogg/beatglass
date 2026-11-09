"""Verify the keyboard shortcut toggles the overlay hide/restore path."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayHideShortcut"')
    block = QML[start:start + 1100]
    for marker in ('sequence: "Ctrl+Alt+H"', "window.overlayHiddenByUser", "showOverlayAndHideControls", "hideOverlayAndRestoreControls"):
        assert marker in block, marker
    print("Overlay hide shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
