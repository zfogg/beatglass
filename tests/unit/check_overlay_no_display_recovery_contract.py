"""Verify no enumerated displays enter overlay recovery-pending state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayDisplayRecoveryPending = window.overlayBridge.screenNames.length === 0 || window.overlayDisplaySelectionRequired'
if marker not in qml:
    raise SystemExit("Missing no-display recovery state")
print("Overlay no-display recovery contract verified (1 check)")
