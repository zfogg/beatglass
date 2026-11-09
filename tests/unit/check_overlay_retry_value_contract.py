"""Verify overlay retry action exposes recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.value: window.overlayDisplayRecoveryPending ? "available; overlay recovery pending" : "available; overlay ready"'
if marker not in qml:
    raise SystemExit("Missing overlay retry value marker")
print("Overlay retry value contract verified (1 check)")
