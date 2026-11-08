"""Verify geometry status name communicates pending recovery."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayGeometryStatus"',
    'Accessible.name: "Overlay geometry status: " + text',
    'window.overlayDisplayRecoveryPending ? " (recovery pending)"',
    'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending" : "")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing geometry accessible-name markers: " + ", ".join(missing))
print(f"Overlay geometry accessible-name contract verified ({len(required)} checks)")
