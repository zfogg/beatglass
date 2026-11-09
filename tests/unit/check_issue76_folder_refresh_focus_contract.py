"""Verify folder refresh returns focus to the Library browser."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Folder contents refreshed.' in QML
    assert 'window.refreshCatalog(); window.announceVisualizationRecovery("Folder contents refreshed."); browser.forceActiveFocus()' in QML
    print("Issue #76 folder refresh focus contract verified (2 checks)")
if __name__ == "__main__": main()
