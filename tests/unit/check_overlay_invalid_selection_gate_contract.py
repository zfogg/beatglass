"""Executable contract for invalid display selection gating."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function persistOverlayDisplaySelection(index, name)")
    block = QML[start:QML.index("    function hideOverlayAndRestoreControls()", start)]
    marker = "index < 0 || index >= window.overlayBridge.screenNames.length"
    section = block[block.index(marker):block.index("        if (!window.overlayBridge.moveToScreen", block.index(marker))]
    assert "overlayDisplaySelectionRequired = true" in section
    assert "overlayDisplay.forceActiveFocus()" in section
    print("Overlay invalid selection gate contract verified (2 checks)")


if __name__ == "__main__":
    main()
