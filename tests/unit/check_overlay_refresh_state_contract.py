"""Verify endpoint refresh exposes recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'text: "Refresh overlay endpoint"',
    'Accessible.value: enabled ? (window.overlayDisplayRecoveryPending ? "refresh available; recovery pending" : "refresh available; ready") : "refresh unavailable"',
    'Accessible.description: enabled ? "Refresh available output endpoints before starting or retrying overlay monitoring."',
    'window.endpointSelectionBridge.refresh()',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay refresh state markers: " + ", ".join(missing))
print(f"Overlay refresh state contract verified ({len(required)} checks)")
