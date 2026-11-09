"""Verify overlay capture status value includes recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'objectName: "overlayCaptureStatus"; Accessible.name: "Overlay capture status: " + text; Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")'
if marker not in qml:
    raise SystemExit("Missing overlay capture status value marker")
print("Overlay capture status value contract verified (1 check)")
