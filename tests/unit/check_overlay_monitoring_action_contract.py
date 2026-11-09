"""Verify overlay settings provide a truthful start/stop monitoring action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayMonitoringAction"')
    block = QML[start:start + 1800]
    for marker in ("Stop overlay monitoring", "Start overlay monitoring", "captureMonitorBridge.start()", "captureMonitorBridge.stop()", "accepted output data"):
        assert marker in block, marker
    print("Overlay monitoring action contract verified (5 checks)")

if __name__ == "__main__":
    main()
