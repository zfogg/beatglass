"""Executable contract for hide action accessibility state."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "hideOverlay"')
    block = QML[start:QML.index('objectName: "quitBeatglass"', start)]
    assert "Accessible.value" in block
    assert "already hidden" in block
    assert "control-window fallback is active" in block
    print("Overlay hide accessibility contract verified (3 checks)")


if __name__ == "__main__":
    main()
