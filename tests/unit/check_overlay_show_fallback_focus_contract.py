"""Executable contract for focus on show fallback paths."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function showOverlayAndHideControls()")
    block = QML[start:QML.index("    function unlockOverlayAndRestoreFocus()", start)]
    assert block.count("overlayDisplay.forceActiveFocus()") >= 3
    assert "DJ overlay unavailable" in block
    assert "DJ overlay visual state unavailable" in block
    print("Overlay show fallback focus contract verified (3 checks)")


if __name__ == "__main__":
    main()
