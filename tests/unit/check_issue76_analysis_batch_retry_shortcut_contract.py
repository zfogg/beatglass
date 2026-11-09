"""Verify keyboard batch retry is gated and result-aware."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"analysisBatchRetryShortcut\"", "sequence: \"Ctrl+Alt+B\"", "window.analysisJobBridge.retryBatch()", "Analysis batch retry queued from the keyboard", "Analysis batch retry was rejected"):
        assert marker in QML, marker
    print("Issue #76 analysis batch retry shortcut contract verified (5 checks)")
if __name__ == "__main__": main()
