"""Verify playback feedback names overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationPlaybackFeedback"',
    'Accessible.name: "Visualization playback feedback, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text',
    'Accessible.value: text',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing playback feedback markers: " + ", ".join(missing))
print(f"Playback feedback name contract verified ({len(required)} checks)")
