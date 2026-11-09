from pathlib import Path

qml = (Path(__file__).parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for control in ("seekBackPlayback", "seekForwardPlayback", "seekToFrame"):
    if f'target: {control}; property: "enabled"' not in qml:
        raise SystemExit(f"Missing selected-source seek guard for {control}")
print("playback seek selection contract verified")
