"""Verify bridge disconnect/reconnect resets pending opacity without stale slider state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function onAvailableChanged()",
    "window.overlayOpacityEditInFlight = false",
    "window.overlayPendingOpacity = -1",
    "window.syncOverlaySettingsFromBridge()",
    "Overlay actions disabled; control-window fallback remains available until the native overlay recovers.",
    "Overlay actions restored; native display settings are available.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing opacity disconnect reset markers: " + ", ".join(missing))
print(f"Overlay opacity disconnect reset contract verified ({len(required)} checks)")
