"""Verify device recovery status names overlay recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationDeviceRecoveryState"',
    'Accessible.name: "Visualization device recovery state, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text',
    '"Capture retry failed · check permission and selected device"',
    '"Capture device ready"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization device name markers: " + ", ".join(missing))
print(f"Visualization device name contract verified ({len(required)} checks)")
