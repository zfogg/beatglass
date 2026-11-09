"""Verify overlay geometry keeps the size setting synchronized after restore."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function syncOverlaySizePreferenceFromGeometry(geometry)",
    "const sizes = [[250, 110], [310, 130], [430, 180]]",
    "window.overlayPreferredSizeIndex = best",
    "window.syncOverlaySizePreferenceFromGeometry(savedGeometry)",
    "window.syncOverlaySizePreferenceFromGeometry(window.overlayBridge.geometry)",
    'objectName: "overlaySize"',
    "currentIndex: window.overlayPreferredSizeIndex",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay size state markers: " + ", ".join(missing))
if qml.count("function syncOverlaySizePreferenceFromGeometry(geometry)") != 1:
    raise SystemExit("overlay size synchronization must have one canonical helper")
print(f"Overlay size state contract verified ({len(required)} checks)")
