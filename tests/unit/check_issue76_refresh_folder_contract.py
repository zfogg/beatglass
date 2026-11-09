"""Executable contract for refreshing the Library folder browser."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "refreshFolder"')
block = qml[start:start + 1000]
for marker in ("files.refresh()", "window.refreshCatalog()", "Folder contents refreshed"):
    assert marker in block, marker
print("Issue #76 folder refresh contract verified")
