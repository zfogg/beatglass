"""Verify selected Library analysis exposes an action-oriented status."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"selectedAnalysisActionStatus\"", "Selected track · analysis in progress", "Selected track · analysis ready", "Selected track · analysis needs retry", "Selected track analysis action status"):
        assert marker in QML, marker
    print("Issue #76 selected analysis action status contract verified (5 checks)")
if __name__ == "__main__": main()
