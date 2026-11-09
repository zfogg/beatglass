"""Executable contract for the Library recursive-folder action."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ('objectName: "addFolderRecursively"', "libraryCatalogBridge.browseRecursive", "recursiveCatalogRows", "recursiveCatalogActive"):
    assert marker in qml, marker
print("Issue #76 recursive library UI contract verified")
