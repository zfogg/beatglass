"""Verify settings synchronization fails closed when visual restore fails."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.announceCaptureRecoveryState("Overlay settings could not be synchronized; control-window fallback remains active.")'
if marker not in qml or 'return visualStateReady || (geometry && geometry.width > 0)' in qml:
    raise SystemExit("Missing fail-closed overlay settings synchronization")
print("Overlay settings sync result contract verified (1 check)")
