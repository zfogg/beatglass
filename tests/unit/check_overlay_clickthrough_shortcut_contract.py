"""Verify the keyboard shortcut toggles native overlay click-through."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayClickThroughShortcut"')
    block = QML[start:start + 1200]
    for marker in ('sequence: "Ctrl+Alt+I"', "setClickThrough(requested)", "press Escape to unlock interaction", "input mode could not be changed"):
        assert marker in block, marker
    print("Overlay click-through shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
