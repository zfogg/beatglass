"""Verify estimate readiness names recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayDataReadiness"',
    'Accessible.name: "Overlay estimate readiness, " + (window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")',
    'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending; snapshot retained" : "; recovery ready")',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay readiness name markers: " + ", ".join(missing))
print(f"Overlay readiness name contract verified ({len(required)} checks)")
