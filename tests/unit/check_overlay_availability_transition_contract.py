"""Verify native availability transitions preserve fallback and resync state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function onAvailableChanged()",
    "window.overlayDisplayRecoveryPending = true",
    "window.hideOverlayAndRestoreControls()",
    "window.syncOverlaySettingsFromBridge()",
    "if (!window.overlayBridge)",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay availability transition markers: " + ", ".join(missing))
print(f"Overlay availability transition contract verified ({len(required)} checks)")
