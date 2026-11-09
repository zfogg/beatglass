"""Verify endpoint selector exposes overlay recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: (currentText !== "" ? "Current capture endpoint: " + currentText + ". Choose the output endpoint used for live capture." : "No capture endpoint selected. Choose the output endpoint used for live capture.") + (window.overlayDisplayRecoveryPending ? " Overlay recovery is pending." : " Overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing endpoint selector recovery marker")
print("Endpoint selector recovery contract verified (1 check)")
