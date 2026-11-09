"""Verify recovery announcement description includes overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Live announcement for keyboard or pointer playback recovery actions; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing visualization announcement description marker")
print("Visualization announcement description contract verified (1 check)")
