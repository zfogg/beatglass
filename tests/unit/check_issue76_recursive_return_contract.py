"""Verify recursive results provide a return-to-browser action."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"returnToFolderBrowser\"", "window.recursiveCatalogActive = false", "Returned to the current folder browser.", "browser.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 recursive return contract verified (4 checks)")
if __name__ == "__main__": main()
