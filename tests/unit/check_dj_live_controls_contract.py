"""Verify the DJ workspace exposes source, monitoring, health, and routing controls."""
from pathlib import Path


qml = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "monitoringControls"',
    'objectName: "toggleMonitoring"',
    'objectName: "monitoringStatusGroup"',
    'objectName: "liveSourceSelector"',
    'objectName: "refreshLiveSources"',
    'objectName: "captureHealthStatus"',
    'objectName: "outputRoutingStatus"',
    'endpointSelectionBridge.select(currentValue.id)',
    'captureMonitorBridge.start()',
    'monitor-only system output capture',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("missing DJ/live control markers: " + ", ".join(missing))
print("DJ/live controls contract: verified")
