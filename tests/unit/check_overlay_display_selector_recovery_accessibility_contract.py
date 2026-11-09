"""Executable contract for display selector recovery accessibility."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "overlayDisplay"')
    block = QML[start:QML.index("                        enabled:", start)]
    assert "Accessible.value" in block
    assert "recovery pending" in block
    assert "Recovery is pending" in block
    print("Overlay display selector recovery accessibility contract verified (3 checks)")


if __name__ == "__main__":
    main()
