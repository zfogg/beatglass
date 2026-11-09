"""Verify Escape clears the selected Library source from the browser."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("Keys.onEscapePressed: { window.clearSelectedAudio(); search.forceActiveFocus() }", "function clearSelectedAudio()"):
        assert marker in QML, marker
    print("Issue #76 browser Escape contract verified (2 checks)")
if __name__ == "__main__": main()
