"""Verify visualization status tooltip includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'ToolTip.text: "Visualization data, generation, and capture status: " + (window.liveBridge.status === "Error" ? "capture error" : (visualizationCanvas.modeHasData ? "ready" : "waiting")) + "; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")'
if marker not in qml:
    raise SystemExit("Missing visualization status tooltip marker")
print("Visualization status tooltip contract verified (1 check)")
