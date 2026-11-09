"""Executable contract for visible batch completion progress."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "analysisBatchProgressBar"')
block = qml[start:start + 1800]
for marker in ("batchCompletedCount", "batchFailedCount", "batchCancelledCount", "Accessible.role: Accessible.ProgressBar"):
    assert marker in block, marker
print("Issue #76 batch progress contract verified")
