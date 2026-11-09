"""Verify overlay exposes a focused capture retry action after errors."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayCaptureRecoveryAction"')
    block = QML[start:start + 1000]
    for marker in ("status === \"Error\"", "Retry overlay capture", "captureMonitorBridge.start()", "fresh accepted snapshot", "Accessible.role: Accessible.Button"):
        assert marker in block, marker
    print("Overlay capture recovery action contract verified (5 checks)")

if __name__ == "__main__":
    main()
