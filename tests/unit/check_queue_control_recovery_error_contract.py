"""Verify single queue progress and batch status share persisted recovery errors."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "analysisQueueProgress"',
    '" · recovery: " + storageRecovery.analysisError',
    'objectName: "analysisBatchStatus"',
    'objectName: "cancelAdmittedAnalysis"',
    'objectName: "retryAdmittedAnalysis"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing queue recovery control markers: " + ", ".join(missing))
print(f"Queue control recovery error contract verified ({len(required)} checks)")
