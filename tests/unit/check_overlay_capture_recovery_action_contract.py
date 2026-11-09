"""Verify the overlay capture recovery action exposes actionable state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayCaptureRecoveryAction"',
    'Accessible.value: enabled ? "retry available; capture status error" : "retry unavailable"',
    '"Retry the selected output capture for the overlay and wait for a fresh accepted snapshot; settings revision "',
    '"Overlay capture retry is unavailable; settings revision "',
    'ToolTip.text: Accessible.description',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay capture recovery markers: " + ", ".join(missing))
print(f"Overlay capture recovery action contract verified ({len(required)} checks)")
