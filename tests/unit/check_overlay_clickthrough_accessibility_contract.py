"""Executable contract for click-through recovery accessibility."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "clickThrough"')
    block = QML[start:QML.index('objectName: "overlayInteractionMode"', start)]
    assert "Accessible.value" in block
    assert "recovery pending" in block
    print("Overlay click-through accessibility contract verified (2 checks)")


if __name__ == "__main__":
    main()
