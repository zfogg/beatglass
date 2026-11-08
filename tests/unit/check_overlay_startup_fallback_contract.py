"""Verify startup marks native overlay loss as pending recovery."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function syncOverlaySettingsFromBridge()",
    "window.overlayDisplayRecoveryPending = true",
    "!window.overlayBridge.available || window.overlayBridge.screenNames.length === 0",
    "Overlay startup state is unavailable; control-window fallback remains active until a display recovers.",
    "window.syncOverlaySettingsFromBridge()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay startup fallback markers: " + ", ".join(missing))
if qml.count("window.overlayDisplayRecoveryPending = true") < 4:
    raise SystemExit("startup, monitor loss, unavailable selection, and failed recovery must stay pending")
print(f"Overlay startup fallback contract verified ({len(required)} checks)")
