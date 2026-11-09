"""Verify playback resume announces the bridge result."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("const resumed = window.playbackBridge.resume()", "Playback resumed; accepted snapshot retained.", "Playback resume failed; the playback state was unchanged."):
        assert marker in QML, marker
    print("Issue #76 playback resume result contract verified (3 checks)")
if __name__ == "__main__": main()
