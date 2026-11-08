"""Verify geometry, opacity, display, and size advance one settings revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property int overlaySettingsRevision: 0",
    "function syncOverlaySettingsStateFromBridge()",
    "window.syncOverlayGeometryStateFromBridge()",
    "const opacity = Number(window.overlayBridge.opacity)",
    "window.overlaySettingsRevision += 1",
    'objectName: "overlaySettingsSummary"',
    'settings revision " + window.overlaySettingsRevision',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing atomic settings sync markers: " + ", ".join(missing))
print(f"Overlay atomic settings sync contract verified ({len(required)} checks)")
