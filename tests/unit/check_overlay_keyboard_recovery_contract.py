"""Verify keyboard recovery guidance remains visible for click-through overlay."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayKeyboardRecoveryStatus"')
    block = QML[start:start + 1100]
    for marker in ("press Escape to unlock overlay", "Ctrl+Alt+U", "overlay controls are interactive", "Accessible.role: Accessible.Status"):
        assert marker in block, marker
    print("Overlay keyboard recovery contract verified (4 checks)")

if __name__ == "__main__":
    main()
