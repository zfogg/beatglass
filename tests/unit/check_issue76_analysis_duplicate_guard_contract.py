"""Executable contract preventing duplicate single-file analysis submissions."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "analyzeSelectedAudio"')
block = qml[start:start + 1400]
for marker in ('status !== "Queued"', 'status !== "Running"', "already active"):
    assert marker in block, marker
print("Issue #76 single-analysis duplicate guard contract verified")
