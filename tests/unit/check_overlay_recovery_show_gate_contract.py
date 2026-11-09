"""Executable contract for show failure selection gating."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    function_start = QML.index("function recoverOverlayAfterDisplayChange()")
    start = QML.index("if (!window.overlayBridge.showOverlay())", function_start)
    block = QML[start:QML.index("            }", start)]
    assert "overlayDisplayRecoveryPending = true" in block
    assert "overlayDisplaySelectionRequired = true" in block
    assert "restoreOverlayFallbackControls" in block
    print("Overlay recovery show gate contract verified (3 checks)")


if __name__ == "__main__":
    main()
