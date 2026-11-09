"""Verify overlay retry description includes recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: "Retry monitoring the selected output source; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing overlay retry description marker")
print("Overlay retry description contract verified (1 check)")
