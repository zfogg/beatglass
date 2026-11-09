"""Verify the keyboard shortcut focuses the unavailable-overlay retry action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayRetryFocusShortcut"')
    block = QML[start:start + 850]
    for marker in ('sequence: "Ctrl+Alt+Shift+Y"', "!window.overlayBridge.available", "retryOverlay.forceActiveFocus()", "Retry overlay control focused"):
        assert marker in block, marker
    print("Overlay retry focus shortcut contract verified (4 checks)")

if __name__ == "__main__":
    main()
