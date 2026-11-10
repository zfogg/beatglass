"""Verify the recursive batch action shows its result count."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'text: "Analyze first files (" + window.recursiveAudioResultCount + ")"' in QML
    print("Issue #76 recursive batch count label contract verified")
if __name__ == "__main__": main()
