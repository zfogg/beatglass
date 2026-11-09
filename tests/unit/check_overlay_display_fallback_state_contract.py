"""Verify stale/no-display selection activates overlay fallback state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayDisplayRecoveryPending = window.overlayBridge.screenNames.length === 0 || window.overlayDisplaySelectionRequired\n        window.overlayLaunchFallbackActive = window.overlayDisplaySelectionRequired'
if marker not in qml:
    raise SystemExit("Missing display fallback state synchronization")
print("Overlay display fallback state contract verified (1 check)")
