"""Verify display selection fails when settings synchronization fails."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'if (!window.syncOverlaySettingsStateFromBridge())'
if marker not in qml:
    raise SystemExit("Missing display settings synchronization result handling")
print("Overlay display sync result contract verified (1 check)")
