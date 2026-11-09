"""Verify the keyboard shortcut cycles native overlay size presets safely."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlaySizeShortcut"')
    block = QML[start:start + 1500]
    for marker in ('sequence: "Ctrl+Alt+O"', "window.overlayPreferredSizeIndex + 1", "window.overlayBridge.resizeOverlay", "persisted size remains active"):
        assert marker in block, marker
    print("Overlay size shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
