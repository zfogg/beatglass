"""Verify playback epoch value includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")'
if marker not in qml:
    raise SystemExit("Missing playback epoch value marker")
print("Visualization epoch value contract verified (1 check)")
