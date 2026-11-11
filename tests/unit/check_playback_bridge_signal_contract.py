"""Verify playback QML consumes the bridge's single changed signal."""
from pathlib import Path

qml = Path(__file__).parents[1].joinpath("..", "src", "desktop", "qml", "Main.qml").resolve().read_text(encoding="utf-8")
start = qml.index("target: window.playbackBridge")
block = qml[start: qml.index("target: window.endpointSelectionBridge", start)]
if "function onChanged()" not in block:
    raise SystemExit("playback bridge connection must consume changed()")
for invalid in ("function onSourceChanged()", "function onEpochChanged()", "function onStateChanged()"):
    if invalid in block:
        raise SystemExit(f"playback bridge registers invalid signal handler: {invalid}")
print("playback bridge signal contract verified")
