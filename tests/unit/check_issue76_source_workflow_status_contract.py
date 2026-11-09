"""Verify selected-source status includes playback and analysis state."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"sourceWorkflowStatus\"", "Source workflow · playback:", "· analysis: ", "window.analysisJobBridge ? window.analysisJobBridge.status"):
        assert marker in QML, marker
    print("Issue #76 source workflow status contract verified (4 checks)")
if __name__ == "__main__": main()
