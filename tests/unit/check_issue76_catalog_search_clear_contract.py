"""Verify Escape clears the catalog filter and restores browser focus."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("Keys.onEscapePressed", 'text = ""', "browser.forceActiveFocus()", "Library filter cleared; all folder entries are visible."):
        assert marker in QML, marker
    print("Issue #76 catalog search clear contract verified (4 checks)")
if __name__ == "__main__": main()
