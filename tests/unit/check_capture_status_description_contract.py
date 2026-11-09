"""Verify DJ capture status exposes overlay recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'objectName: "captureStatus"; Accessible.name: "Capture monitor status: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text + ". " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing capture status recovery description marker")
print("Capture status description contract verified (1 check)")
