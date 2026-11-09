"""Verify live monitoring status names recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayLiveMonitoringState"',
    'Accessible.name: "Overlay live monitoring state, " + (window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")',
    'window.liveBridge.status === "Tracking"',
    '"Overlay monitoring unavailable"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay live name markers: " + ", ".join(missing))
print(f"Overlay live name contract verified ({len(required)} checks)")
