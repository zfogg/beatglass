"""Verify successful settings synchronization clears recovery-pending state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'if (visualStateReady) {\n            window.overlayDisplayRecoveryPending = false'
if marker not in qml:
    raise SystemExit("Missing successful sync recovery clear")
print("Overlay sync recovery clear contract verified (1 check)")
