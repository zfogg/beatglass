"""Verify analysis submission preserves focus on its recovery action."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"analyzeSelectedAudio\"", "submitSelectedFile(window.selectedAudioUrl.toLocalFile())", "analyzeSelectedAudio.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 analysis submit focus contract verified (3 checks)")
if __name__ == "__main__": main()
