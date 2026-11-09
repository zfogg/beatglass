"""Verify visualization empty-state description includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: text + " " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing visualization empty description marker")
print("Visualization empty description contract verified (1 check)")
