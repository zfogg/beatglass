"""Executable contract for routing recovery description."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "overlayRoutingControls"')
    block = QML[start:QML.index('objectName: "clickThrough"', start)]
    assert "Accessible.description" in block
    assert "Recovery is pending" in block
    print("Overlay routing recovery description contract verified (2 checks)")


if __name__ == "__main__":
    main()
