"""Executable contract for display recovery focus."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function recoverOverlayAfterDisplayChange()")
    block = QML[start:QML.index("    function forgetUnavailableOverlayDisplay()", start)]
    assert block.count("overlayDisplay.forceActiveFocus()") >= 2
    assert "persisted visual state could not be reapplied" in block
    assert "overlayDisplaySelectionRequired = true" in block
    print("Overlay display recovery focus contract verified (3 checks)")


if __name__ == "__main__":
    main()
