"""Verify the results shortcut focuses the active Library result list."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"libraryResultsShortcut\"", "sequence: \"Ctrl+Alt+Shift+L\"", "window.recursiveCatalogActive", "recursiveResultsView.forceActiveFocus()", "browser.forceActiveFocus()"):
        assert marker in QML, marker
    print("Issue #76 Library results shortcut contract verified (5 checks)")
if __name__ == "__main__": main()
