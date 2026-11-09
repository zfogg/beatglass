"""Verify selected-track analysis exposes a retry action."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"selectedAnalysisRetry\"", "window.analysisJobBridge.canRetry", "Retry selected analysis", "window.retryVisualizationAnalysis()"):
        assert marker in QML, marker
    print("Issue #76 selected analysis retry action contract verified (4 checks)")
if __name__ == "__main__": main()
