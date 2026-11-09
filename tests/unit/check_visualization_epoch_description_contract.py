"""Verify playback epoch description includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Live playback source and epoch alignment for the accepted visualization snapshot; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing playback epoch description marker")
print("Visualization epoch description contract verified (1 check)")
