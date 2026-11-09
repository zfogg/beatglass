"""Verify the overlay communicates whether an accepted snapshot is ready."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayDataReadiness"')
    block = QML[start:start + 1100]
    for marker in (
        "accepted snapshot ready",
        "waiting for accepted snapshot",
        "current accepted visualization snapshot",
        "replacement snapshot",
        "Accessible.role: Accessible.Status",
    ):
        assert marker in block, marker
    print("Overlay data-readiness contract verified (5 checks)")

if __name__ == "__main__":
    main()
