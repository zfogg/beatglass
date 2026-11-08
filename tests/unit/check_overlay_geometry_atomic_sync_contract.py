"""Verify DPI/layout geometry changes update display, size, and summary revision together."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property int overlayGeometryRevision: 0",
    "function syncOverlayGeometryStateFromBridge()",
    "window.overlayPreferredDisplayName = window.overlayBridge.displayName",
    "window.overlayPreferredScreenIndex = window.overlayBridge.screenIndex",
    "window.syncOverlaySizePreferenceFromGeometry(geometry)",
    "window.overlayGeometryRevision += 1",
    'objectName: "overlayGeometryStatus"',
    'geometry revision " + window.overlayGeometryRevision',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing atomic geometry sync markers: " + ", ".join(missing))
print(f"Overlay atomic geometry sync contract verified ({len(required)} checks)")
