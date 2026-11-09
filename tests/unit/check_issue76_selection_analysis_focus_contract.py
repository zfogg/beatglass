"""Verify selecting a Library source moves focus to its analysis action."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("selectedAudioUrl = url", "analyzeSelectedAudio.forceActiveFocus()", "prior analysis presentation is stale"):
        assert marker in QML, marker
    print("Issue #76 selection analysis focus contract verified (3 checks)")
if __name__ == "__main__": main()
