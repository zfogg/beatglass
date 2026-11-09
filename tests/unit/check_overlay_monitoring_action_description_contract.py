"""Verify the overlay monitoring action announces recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayDisplayRecoveryPending ? " Overlay recovery is pending." : " Overlay is ready."'
if marker not in qml:
    raise SystemExit("Missing overlay monitoring action recovery description")
print("Overlay monitoring action description contract verified (1 check)")
