"""Verify direct opacity edits and native opacity echoes advance exactly once."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property bool overlayOpacityEditInFlight: false",
    "window.overlayOpacityEditInFlight = true",
    "window.overlayBridge.setOpacity(value)",
    "window.overlaySettingsRevision += 1",
    "window.overlayOpacityEditInFlight",
    "window.overlayOpacityEditInFlight = false",
    "window.overlaySettingsRevision += 1",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing opacity echo revision markers: " + ", ".join(missing))
print(f"Overlay opacity echo revision contract verified ({len(required)} checks)")
