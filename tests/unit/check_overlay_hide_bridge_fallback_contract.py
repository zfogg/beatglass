"""Executable contract for hide fallback when native bridge is absent."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function hideOverlayAndRestoreControls()")
    block = QML[start:QML.index("    function quitApplication()", start)]
    for fragment in ("window.overlayHiddenByUser = true", "if (window.overlayBridge)", "control window restored as the fallback", "window.show()"):
        assert fragment in block, f"missing contract fragment: {fragment}"
    print("Overlay hide bridge fallback contract verified (4 checks)")


if __name__ == "__main__":
    main()
