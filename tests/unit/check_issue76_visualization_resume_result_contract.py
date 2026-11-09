"""Verify visualization resume shortcut reports the bridge result."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"visualizationResumeShortcut\"", "const resumed = window.playbackBridge.resume()", "Playback resume from keyboard shortcut failed"):
        assert marker in QML, marker
    print("Issue #76 visualization resume result contract verified (3 checks)")
if __name__ == "__main__": main()
