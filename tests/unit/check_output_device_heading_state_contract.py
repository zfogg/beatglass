"""Verify output device heading announces overlay recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.name: "Output device settings, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")'
if marker not in qml:
    raise SystemExit("Missing output device heading state marker")
print("Output device heading state contract verified (1 check)")
