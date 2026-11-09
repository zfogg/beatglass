"""Verify the keyboard shortcut retries an unavailable overlay safely."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayRetryShortcut"')
    block = QML[start:start + 950]
    for marker in ('sequence: "Ctrl+Alt+Y"', "!window.overlayBridge.available", "showOverlayAndHideControls", "control window remains visible"):
        assert marker in block, marker
    print("Overlay retry shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
