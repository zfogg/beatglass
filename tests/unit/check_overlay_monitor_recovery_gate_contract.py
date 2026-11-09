"""Executable contract for monitor recovery visual-state gating."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function recoverOverlayAfterDisplayChange()")
    block = QML[start:QML.index("    function forgetUnavailableOverlayDisplay()", start)]
    marker = "persisted visual state could not be reapplied"
    assert marker in block
    assert "overlayDisplaySelectionRequired = true" in block
    assert block.index("overlayDisplaySelectionRequired = true") < block.index(marker)
    print("Overlay monitor recovery gate contract verified (3 checks)")


if __name__ == "__main__":
    main()
