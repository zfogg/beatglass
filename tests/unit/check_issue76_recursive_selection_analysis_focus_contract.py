"""Verify recursive catalog selection focuses analysis."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Selected recursive audio source ' in QML
    assert 'analyzeSelectedAudio.forceActiveFocus()' in QML
    print("Issue #76 recursive selection analysis focus contract verified (2 checks)")
if __name__ == "__main__": main()
