"""Verify accepting a Library filter selects the Library tab."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'onAccepted: { navigation.currentIndex = 0; window.announceVisualizationRecovery' in QML
    print("Issue #76 search accept navigation contract verified")
if __name__ == "__main__": main()
