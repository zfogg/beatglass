"""Executable contract for hidden-by-user monitor recovery."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("if (window.overlayDisplayRecoveryPending)")
    block = QML[start:QML.index("    function forgetUnavailableOverlayDisplay()", start)]
    assert "if (window.overlayHiddenByUser)" in block
    assert "remains hidden by user request" in block
    assert "Use Restore overlay when ready" in block
    assert block.index("overlayHiddenByUser") < block.index("showOverlay()")
    print("Overlay hidden recovery contract verified (4 checks)")


if __name__ == "__main__":
    main()
