"""Verify overlay recovery exposes the next safe action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayRecoveryActionStatus"')
    block = QML[start:start + 1200]
    for marker in ("waiting for a compatible display", "choose or reconnect a display", "Restore overlay uses the persisted display", "Accessible.role: Accessible.Status"):
        assert marker in block, marker
    print("Overlay recovery action status contract verified (4 checks)")

if __name__ == "__main__":
    main()
