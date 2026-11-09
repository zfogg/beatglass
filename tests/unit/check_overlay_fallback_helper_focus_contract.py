"""Executable contract for shared overlay fallback focus."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index("function restoreOverlayFallbackControls(reason)")
    block = QML[start:QML.index("    function showOverlayAndHideControls()", start)]
    for fragment in ("window.show()", "window.requestActivate()", "overlayDisplay.forceActiveFocus()"):
        assert fragment in block, f"missing contract fragment: {fragment}"
    print("Overlay fallback helper focus contract verified (3 checks)")


if __name__ == "__main__":
    main()
