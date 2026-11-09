"""Verify the keyboard shortcut resets opacity through the native bridge."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayOpacityResetShortcut"')
    block = QML[start:start + 1400]
    for marker in ('sequence: "Ctrl+Alt+Shift+O"', "const resetOpacity = 0.9", "setOpacity(resetOpacity)", "persisted opacity remains active"):
        assert marker in block, marker
    print("Overlay opacity reset contract verified (4 checks)")

if __name__ == "__main__":
    main()
