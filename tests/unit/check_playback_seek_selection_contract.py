from pathlib import Path

qml = (Path(__file__).parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
if "!window.playbackMatchesSelection" not in qml:
    raise SystemExit("Visualization seek must require selected-source playback alignment")
print("playback seek selection contract verified")
