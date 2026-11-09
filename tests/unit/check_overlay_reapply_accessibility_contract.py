"""Executable contract for Reapply action recovery accessibility."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "reapplyOverlaySettings"')
    block = QML[start:QML.index('objectName: "unlockOverlay"', start)]
    assert "Accessible.value" in block
    assert "available; recovery pending" in block
    print("Overlay reapply accessibility contract verified (2 checks)")


if __name__ == "__main__":
    main()
