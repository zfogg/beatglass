"""Verify endpoint status names recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayEndpointStatus"',
    'Accessible.name: "Overlay capture endpoint status, " + (window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")',
    'Accessible.value: text',
    '"Overlay endpoint: none selected"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay endpoint name markers: " + ", ".join(missing))
print(f"Overlay endpoint name contract verified ({len(required)} checks)")
