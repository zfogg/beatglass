"""Contract checks that recursive analysis invalidates stale evidence."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    start = QML.index('objectName: "analyzeRecursiveResults"')
    block = QML[start:QML.index('objectName: "returnToFolderBrowser"', start)]
    assert "const accepted = window.analysisJobBridge.submitSelectedFiles(paths)" in block
    assert "if (accepted > 0)" in block
    assert "clearVisualizationForCaptureState" in block
    assert "Recursive analysis queued" in block
if __name__ == "__main__":
    main()
