"""Verify missing overlay bridge enters recovery-pending state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'if (!window.overlayBridge) {\n            window.overlayDisplayRecoveryPending = true'
if marker not in qml:
    raise SystemExit("Missing no-bridge recovery state")
print("Overlay no-bridge recovery contract verified (1 check)")
