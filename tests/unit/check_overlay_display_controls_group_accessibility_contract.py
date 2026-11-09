"""Executable contract for display controls group recovery state."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "overlayDisplayControls"')
    block = QML[start:QML.index('objectName: "overlayDisplay"', start)]
    assert "recovery pending" in block
    assert "hidden by user" in block
    assert "Recovery is pending" in block
    print("Overlay display controls group accessibility contract verified (3 checks)")


if __name__ == "__main__":
    main()
