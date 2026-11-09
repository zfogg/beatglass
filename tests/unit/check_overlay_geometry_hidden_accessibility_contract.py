"""Executable contract for hidden overlay geometry accessibility."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "overlayGeometryStatus"')
    block = QML[start:start + 1500]
    assert "geometry revision" in block
    assert "hidden by user" in block
    assert "overlay is hidden by user request" in block
    print("Overlay geometry hidden accessibility contract verified (3 checks)")


if __name__ == "__main__":
    main()
