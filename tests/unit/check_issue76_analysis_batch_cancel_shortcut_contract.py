"""Verify keyboard batch cancellation is gated and announced."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"analysisBatchCancelShortcut\"", "sequence: \"Ctrl+Alt+K\"", "window.analysisJobBridge.cancelBatch()", "Analysis batch cancelled from the keyboard"):
        assert marker in QML, marker
    print("Issue #76 analysis batch cancel shortcut contract verified (4 checks)")
if __name__ == "__main__": main()
