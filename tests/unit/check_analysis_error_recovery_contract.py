"""Verify failed queue work exposes a retryable user-facing diagnostic."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "Q_PROPERTY(QString analysisError",
    "Analysis failed; retryable work was preserved for recovery.",
    "analysisErrorChanged",
    'objectName: "analysisQueueError"',
    "storageRecovery.analysisError",
]
missing = [marker for marker in required if marker not in header + source + qml]
if missing:
    raise SystemExit("Missing analysis error recovery markers: " + ", ".join(missing))
print(f"Analysis error recovery contract verified ({len(required)} checks)")
