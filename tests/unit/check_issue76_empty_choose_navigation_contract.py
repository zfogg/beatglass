"""Verify empty-state folder recovery selects the Library tab."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'navigation.currentIndex = 0; window.announceVisualizationRecovery("Choose a Library folder dialog opened from the empty state.")' in QML
    print("Issue #76 empty choose navigation contract verified")
if __name__ == "__main__": main()
