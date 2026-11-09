"""Verify Escape returns from recursive results to the folder browser."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("id: recursiveResultsView", "Keys.onEscapePressed", "window.recursiveCatalogActive = false", "browser.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 recursive Escape contract verified (4 checks)")
if __name__ == "__main__": main()
