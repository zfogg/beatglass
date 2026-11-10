"""Verify recursive batch announcements include queue capacity."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'queue capacity " + window.analysisJobBridge.batchCapacity' in QML
    print("Issue #76 recursive batch capacity contract verified")
if __name__ == "__main__": main()
