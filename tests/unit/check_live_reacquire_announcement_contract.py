"""Verify starting and reacquiring live states announce next actions."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    assert "Output monitoring is starting" in QML
    assert "first accepted visualization snapshot" in QML
    assert "Output monitoring is reacquiring" in QML
    assert "previous visualization selection was cleared" in QML
    print("Live reacquire announcement contract verified (4 checks)")

if __name__ == "__main__":
    main()
