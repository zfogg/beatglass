"""Verify audio capture settings communicate overlay recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Choose the output endpoint and device used for live monitoring. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing audio capture group recovery marker")
print("Audio capture group recovery contract verified (1 check)")
