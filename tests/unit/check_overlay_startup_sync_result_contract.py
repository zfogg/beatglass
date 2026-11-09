"""Verify startup enters fallback when native settings sync fails."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'const synced = window.syncOverlaySettingsFromBridge()\n            if (!synced)'
if marker not in qml:
    raise SystemExit("Missing startup sync result handling")
print("Overlay startup sync result contract verified (1 check)")
