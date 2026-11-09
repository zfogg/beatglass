"""Verify endpoint refresh communicates overlay recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'Accessible.description: enabled ? "Refresh available output endpoints before starting or retrying overlay monitoring. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")'
if marker not in qml:
    raise SystemExit("Missing endpoint refresh recovery marker")
print("Overlay endpoint refresh recovery contract verified (1 check)")
