"""Verify audio capture settings expose endpoint count and recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.value: (window.endpointSelectionBridge ? window.endpointSelectionBridge.endpoints.length : 0) + " output endpoints" + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")'
if marker not in qml:
    raise SystemExit("Missing audio capture group value marker")
print("Audio capture group value contract verified (1 check)")
