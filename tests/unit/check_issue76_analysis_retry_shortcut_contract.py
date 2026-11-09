"""Verify keyboard analysis retry is gated and routed through the shared path."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"analysisRetryShortcut\"", "sequence: \"Ctrl+Alt+Y\"", "window.analysisJobBridge.canRetry", "onActivated: window.retryVisualizationAnalysis()"):
        assert marker in QML, marker
    print("Issue #76 analysis retry shortcut contract verified (4 checks)")
if __name__ == "__main__": main()
