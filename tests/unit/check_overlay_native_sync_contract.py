"""Verify startup synchronizes display, geometry, size, and opacity from native state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function syncOverlaySettingsFromBridge()",
    "window.syncOverlaySettingsFromBridge()",
    "window.restoreOverlayDisplaySelectionFromBridge()",
    "const visualStateReady = window.restoreOverlayVisualStateFromBridge()",
    "Overlay settings synchronized from native state.",
    "window.syncOverlaySizePreferenceFromGeometry(savedGeometry)",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing native overlay sync markers: " + ", ".join(missing))
if qml.count("function syncOverlaySettingsFromBridge()") != 1:
    raise SystemExit("native overlay synchronization must have one canonical helper")
print(f"Overlay native synchronization contract verified ({len(required)} checks)")
