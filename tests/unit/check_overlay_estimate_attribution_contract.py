"""Verify the overlay labels estimates as combined output, not a deck track."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayEstimateAttribution"')
    block = QML[start:start + 900]
    for marker in (
        "Estimate source: selected system output",
        "combined mix",
        "Track and deck metadata are unavailable",
        "Accessible.role: Accessible.StaticText",
    ):
        assert marker in block, marker
    print("Overlay estimate attribution contract verified (4 checks)")

if __name__ == "__main__":
    main()
