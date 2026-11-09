"""Verify invalid native settings enter overlay recovery-pending state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayDisplayRecoveryPending = true\n            return false'
if marker not in qml:
    raise SystemExit("Missing settings recovery-pending state")
print("Overlay settings recovery state contract verified (1 check)")
