"""Verify recursive batch announcements include aggregate status."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'status " + window.analysisJobBridge.status + ".' in QML
    print("Issue #76 recursive batch status contract verified")
if __name__ == "__main__": main()
