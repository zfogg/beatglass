"""Verify overlay settings can refresh the output endpoint list."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayEndpointRefreshAction"')
    block = QML[start:start + 1000]
    for marker in ("Refresh overlay endpoint", "endpointSelectionBridge.refresh()", "endpointSelector.forceActiveFocus()", "before retrying monitoring", "Accessible.role: Accessible.Button"):
        assert marker in block, marker
    print("Overlay endpoint refresh action contract verified (5 checks)")

if __name__ == "__main__":
    main()
