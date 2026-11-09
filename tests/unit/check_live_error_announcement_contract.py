"""Verify live capture errors announce a concrete recovery action."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    assert 'status === "Error"' in QML
    assert "Output monitoring reported an error" in QML
    assert "retry monitoring" in QML
    assert "permission state" in QML
    print("Live error announcement contract verified (4 checks)")

if __name__ == "__main__":
    main()
