"""Executable contract for hidden display status accessibility."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "overlayDisplayStatus"')
    block = QML[start:start + 1200]
    assert "hidden by user" in block
    assert "use Restore overlay when ready" in block
    print("Overlay display hidden accessibility contract verified (2 checks)")


if __name__ == "__main__":
    main()
