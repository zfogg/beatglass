"""Verify live tracking transitions announce visualization readiness."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index("function onStatusChanged()", QML.index("target: window.liveBridge"))
    block = QML[start:start + 1500]
    assert 'status === "Tracking"' in block
    assert "Output monitoring is tracking" in block
    assert "accepted data arrives" in block
    print("Live tracking announcement contract verified (3 checks)")

if __name__ == "__main__":
    main()
