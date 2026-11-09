"""Verify stale saved displays enter recovery-pending state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayDisplayRecoveryPending = window.overlayDisplaySelectionRequired'
if marker not in qml:
    raise SystemExit("Missing display recovery pending state synchronization")
print("Overlay display recovery state contract verified (1 check)")
