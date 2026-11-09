"""Executable contract for verified batch publication presentation."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "analysisBatchPublicationSummary"')
block = qml[start:start + 1400]
for marker in ("batchResultsPublished", "batchQueuedCount", "batchFailedCount", "batchCancelledCount", "after verification"):
    assert marker in block, marker
print("Issue #76 batch publication summary contract verified")
