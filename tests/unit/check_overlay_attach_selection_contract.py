"""Verify failed native attachment requires display recovery selection."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayDisplayRecoveryPending = true\n            window.overlayDisplaySelectionRequired = true'
if marker not in qml:
    raise SystemExit("Missing attach display selection recovery state")
print("Overlay attach selection contract verified (1 check)")
