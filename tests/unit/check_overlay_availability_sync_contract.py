"""Verify availability recovery does not claim success when settings sync fails."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ("const synced = window.syncOverlaySettingsFromBridge()", "if (synced)", "window.overlayLaunchFallbackActive = true"):
    if marker not in qml:
        raise SystemExit(f"Missing availability sync marker: {marker}")
print("Overlay availability sync contract verified (3 checks)")
