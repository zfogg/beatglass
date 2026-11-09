"""Verify estimate attribution names recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayEstimateAttribution"',
    'Accessible.name: "Overlay estimate attribution, " + (window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")',
    'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending; source retained" : "; source ready")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay attribution name markers: " + ", ".join(missing))
print(f"Overlay attribution name contract verified ({len(required)} checks)")
