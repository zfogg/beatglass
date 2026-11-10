"""Verify the recursive batch label communicates its two-file bound."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'text: "Analyze first files (up to 2 of " + window.recursiveAudioResultCount + ")"' in QML
    print("Issue #76 recursive batch bound label contract verified")
if __name__ == "__main__": main()
