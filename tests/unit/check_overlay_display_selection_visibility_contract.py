"""Verify display selection clears stale availability-loss visibility intent."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayLaunchFallbackActive = false\n        window.overlayWasVisibleBeforeAvailabilityLoss = false\n        window.overlayDisplaySelectionRequired = false'
if marker not in qml:
    raise SystemExit("Missing display-selection visibility reset")
print("Overlay display-selection visibility contract verified (1 check)")
