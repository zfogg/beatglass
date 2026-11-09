"""Verify live monitoring group communicates overlay recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Current output monitoring state and the next action needed to produce live estimates. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing live monitoring group recovery marker")
print("Live monitoring group recovery contract verified (1 check)")
