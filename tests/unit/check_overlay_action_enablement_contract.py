"""Verify unavailable overlay actions identify fallback instead of claiming success."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "showOverlay"',
    'objectName: "restoreOverlay"',
    'objectName: "reapplyOverlaySettings"',
    'Accessible.value: enabled ? "available" : "unavailable; control-window fallback active"',
    'Native overlay unavailable; control-window fallback remains available',
    'Overlay unavailable; control-window fallback is active',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay action enablement markers: " + ", ".join(missing))
print(f"Overlay action enablement contract verified ({len(required)} checks)")
