"""Verify geometry status exposes pending recovery in its accessible value."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayGeometryStatus"',
    'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending" : "")',
    'Accessible.role: Accessible.Status',
    'Persisted overlay position and size unavailable; control-window fallback remains active.',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing geometry recovery value markers: " + ", ".join(missing))
print(f"Overlay geometry recovery value contract verified ({len(required)} checks)")
