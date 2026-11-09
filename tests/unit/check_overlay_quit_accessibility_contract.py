"""Executable contract for quit action accessibility."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "quitBeatglass"')
    block = QML[start:QML.index('objectName: "overlayRoutingControls"', start)]
    assert "Accessible.value" in block
    assert "overlay settings retained" in block
    print("Overlay quit accessibility contract verified (2 checks)")


if __name__ == "__main__":
    main()
