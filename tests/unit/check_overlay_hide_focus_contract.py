"""Executable contract for focus after hiding the overlay."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function hideOverlayAndRestoreControls()")
    block = QML[start:QML.index("    function quitApplication()", start)]
    assert "window.show()" in block
    assert "window.requestActivate()" in block
    assert "hideOverlay.forceActiveFocus()" in block
    print("Overlay hide focus contract verified (3 checks)")


if __name__ == "__main__":
    main()
