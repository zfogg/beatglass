"""Verify live tracking transitions announce visualization readiness."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    block = QML
    assert 'status === "Tracking"' in block
    assert "Output monitoring is tracking" in block
    assert "accepted data arrives" in block
    print("Live tracking announcement contract verified (3 checks)")

if __name__ == "__main__":
    main()
