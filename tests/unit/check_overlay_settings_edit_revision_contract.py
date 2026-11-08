"""Verify direct display/size/opacity edits advance the coherent settings revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "window.overlaySettingsRevision += 1",
    "window.overlayBridge.setOpacity(value)",
    "window.overlayBridge.resizeOverlay(sizes[currentIndex][0], sizes[currentIndex][1])",
    "DJ overlay display selection persisted for ",
    "objectName: \"overlayOpacity\"",
    "objectName: \"overlaySize\"",
    "objectName: \"overlayDisplay\"",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing settings edit revision markers: " + ", ".join(missing))
if qml.count("window.overlaySettingsRevision += 1") < 3:
    raise SystemExit("display, size, and opacity edits must each advance settings revision")
print(f"Overlay settings edit revision contract verified ({len(required)} checks)")
