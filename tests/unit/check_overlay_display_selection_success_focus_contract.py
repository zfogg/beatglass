"""Executable contract for successful display selection focus."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function persistOverlayDisplaySelection(index, name)")
    block = QML[start:QML.index("    function hideOverlayAndRestoreControls()", start)]
    marker = "DJ overlay display selection persisted"
    assert marker in block
    assert block.index("overlayDisplay.forceActiveFocus()", block.index(marker)) > block.index(marker)
    print("Overlay display selection success focus contract verified (2 checks)")


if __name__ == "__main__":
    main()
