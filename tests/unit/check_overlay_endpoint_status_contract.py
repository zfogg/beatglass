"""Verify overlay reports the selected output endpoint and its limits."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayEndpointStatus"')
    block = QML[start:start + 1200]
    for marker in ("selectedEndpoint", "Overlay endpoint: none selected", "individual decks", "Choose an output endpoint", "Accessible.role: Accessible.Status"):
        assert marker in block, marker
    print("Overlay endpoint status contract verified (5 checks)")

if __name__ == "__main__":
    main()
