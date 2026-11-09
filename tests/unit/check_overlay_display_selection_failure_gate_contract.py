"""Executable contract for display selection failure action gating."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function persistOverlayDisplaySelection(index, name)")
    block = QML[start:QML.index("    function hideOverlayAndRestoreControls()", start)]
    assert block.count("overlayDisplaySelectionRequired = true") >= 2
    assert "Overlay display selection could not be applied" in block
    assert "persisted visual state could not be reapplied" in block
    print("Overlay display selection failure gate contract verified (3 checks)")


if __name__ == "__main__":
    main()
