"""Verify the overlay monitoring action exposes live state and recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayMonitoringAction"',
    'Accessible.value: (window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable")',
    'window.overlayDisplayRecoveryPending ? "; recovery pending" : "; ready"',
    'window.captureMonitorBridge.status === "Monitoring"',
    'ToolTip.text: Accessible.description + " Current settings revision: " + window.overlaySettingsRevision + "."',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay monitoring action state markers: " + ", ".join(missing))
print(f"Overlay monitoring action state contract verified ({len(required)} checks)")
