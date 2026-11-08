"""Verify bridge/display transitions announce action enablement changes."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function onAvailableChanged()",
    "Overlay actions disabled; control-window fallback remains available until the native overlay recovers.",
    "Overlay actions restored; native display settings are available.",
    "overlay actions disabled and controls restored until a display is available.",
    "choose a display to re-enable overlay actions and resume monitoring.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay action transition announcement markers: " + ", ".join(missing))
print(f"Overlay action transition announcement contract verified ({len(required)} checks)")
