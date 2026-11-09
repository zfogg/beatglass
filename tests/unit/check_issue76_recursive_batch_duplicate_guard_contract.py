"""Executable contract preventing duplicate recursive batch submissions."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "analyzeRecursiveResults"')
block = qml[start:start + 1200]
for marker in ('status !== "Queued"', 'status !== "Running"', "already active"):
    assert marker in block, marker
print("Issue #76 recursive batch duplicate guard contract verified")
