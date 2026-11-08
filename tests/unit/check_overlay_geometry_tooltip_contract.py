"""Verify geometry tooltip reflects pending recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayGeometryStatus"',
    'ToolTip.text: window.overlayDisplayRecoveryPending',
    '"Current persisted overlay position and size; recovery is pending"',
    '"Current persisted overlay position and size"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing geometry tooltip markers: " + ", ".join(missing))
print(f"Overlay geometry tooltip contract verified ({len(required)} checks)")
