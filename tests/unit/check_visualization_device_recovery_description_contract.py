"""Verify visualization device recovery status states overlay readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Visible no-device, permission, retry-failure, and reacquisition guidance for visualization capture. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing visualization device recovery description marker")
print("Visualization device recovery description contract verified (1 check)")
