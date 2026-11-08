"""Verify explicit display selection clears or sets recovery state coherently."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function forgetUnavailableOverlayDisplay()",
    "window.overlayDisplaySelectionRequired = true",
    "window.overlayDisplayRecoveryPending = true",
    "function persistOverlayDisplaySelection(index, name)",
    "window.overlayDisplaySelectionRequired = false",
    "window.overlayDisplayRecoveryPending = false",
    "DJ overlay display selection persisted for ",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing display selection state markers: " + ", ".join(missing))
if qml.count("window.overlayDisplayRecoveryPending = false") < 2:
    raise SystemExit("successful recovery and explicit selection must both clear pending state")
print(f"Overlay display selection state contract verified ({len(required)} checks)")
