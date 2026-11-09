"""Verify overlay hidden/available state and recovery guidance are visible."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayVisibilityStatus"')
    block = QML[start:start + 1000]
    for marker in ("hidden by user", "Overlay visibility: available", "Restore overlay", "preserves monitoring", "Accessible.role: Accessible.Status"):
        assert marker in block, marker
    print("Overlay visibility status contract verified (5 checks)")

if __name__ == "__main__":
    main()
