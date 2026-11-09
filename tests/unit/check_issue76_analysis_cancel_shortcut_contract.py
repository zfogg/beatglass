"""Verify keyboard analysis cancellation is gated and announced."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"analysisCancelShortcut\"", "sequence: \"Ctrl+Alt+C\"", "window.analysisJobBridge.canCancel", "window.analysisJobBridge.cancel()", "Analysis cancelled from the keyboard"):
        assert marker in QML, marker
    print("Issue #76 analysis cancel shortcut contract verified (5 checks)")
if __name__ == "__main__": main()
