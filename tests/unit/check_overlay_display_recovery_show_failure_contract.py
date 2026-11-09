"""Executable contract for native show failure during display recovery."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("if (window.overlayDisplayRecoveryPending)")
    block = QML[start:QML.index("    function forgetUnavailableOverlayDisplay()", start)]
    assert "if (!window.overlayBridge.showOverlay())" in block
    assert "window.overlayDisplayRecoveryPending = true" in block
    assert "restoreOverlayFallbackControls" in block
    assert "choose another display or retry" in block
    print("Overlay display recovery show failure contract verified (4 checks)")


if __name__ == "__main__":
    main()
