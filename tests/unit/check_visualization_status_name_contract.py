"""Verify visualization status group names overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationStatusGroup"',
    'Accessible.name: "Visualization data and capture status, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'Accessible.value: window.liveBridge.status === "Error"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization status markers: " + ", ".join(missing))
print(f"Visualization status name contract verified ({len(required)} checks)")
