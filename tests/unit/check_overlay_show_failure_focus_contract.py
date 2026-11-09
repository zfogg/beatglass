"""Executable contract for focus after native overlay show failure."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("if (!window.overlayBridge.showOverlay())")
    block = QML[start:QML.index("        }", start)]
    assert "overlayDisplayRecoveryPending = true" in block
    assert "restoreOverlayFallbackControls" in block
    assert "overlayDisplay.forceActiveFocus()" in block
    print("Overlay show failure focus contract verified (3 checks)")


if __name__ == "__main__":
    main()
