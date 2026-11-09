"""Verify the overlay exposes truthful live-monitoring state."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayLiveMonitoringState"')
    block = QML[start:start + 1200]
    for marker in (
        'window.liveBridge.status',
        'status === "Tracking"',
        'live estimates updating',
        'monitored combined output',
        'Accessible.role: Accessible.Status',
    ):
        assert marker in block, marker
    print("Overlay live-monitoring state contract verified (5 checks)")

if __name__ == "__main__":
    main()
