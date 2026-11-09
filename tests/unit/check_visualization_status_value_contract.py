"""Verify visualization status value includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.value: (window.liveBridge.status === "Error" ? "capture error" : (window.liveBridge.status === "Starting" || window.liveBridge.status === "Reacquiring" ? window.liveBridge.status.toLowerCase() : (visualizationCanvas.modeHasData ? "ready" : "waiting"))) + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")'
if marker not in qml:
    raise SystemExit("Missing visualization status value marker")
print("Visualization status value contract verified (1 check)")
