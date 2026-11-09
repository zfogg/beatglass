"""Verify persisted retry selection is gated by explicit retryable state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    'objectName: "retryPersistedRecovery"',
    'storageRecovery.selectedRunId !== "" && storageRecovery.analysisRetryable',
    "Q_PROPERTY(bool analysisRetryable",
    "retrySelectedRun()",
    "retryableRunIds",
]
missing = [marker for marker in required if marker not in qml + bridge]
if missing:
    raise SystemExit("Missing persisted retry action markers: " + ", ".join(missing))
if "Retry rejected: no persisted run selected" not in source:
    raise SystemExit("Retry without selection must report a rejection")
if "Discard rejected: no persisted run selected" not in source:
    raise SystemExit("Discard without selection must report a rejection")
if "Discard rejected: no persisted runs selected" not in source:
    raise SystemExit("Batch discard without selection must report a rejection")
if "Discarding persisted recovery" not in source:
    raise SystemExit("Batch discard must publish a transition status")
if "Retrying persisted recovery" not in source:
    raise SystemExit("Persisted retry must publish a transition status")
if "analysisResult_.clear()" not in source[source.index("void StorageRecoveryBridge::retrySelectedRun"):]:
    raise SystemExit("Persisted retry must clear stale result before requeue")
retry_source = source[source.index("void StorageRecoveryBridge::retrySelectedRun"):]
if "analysisError_.clear()" not in retry_source or "analysisRetryable_ = false" not in retry_source:
    raise SystemExit("Persisted retry must clear stale recovery state")
if source.count("Discarding persisted recovery") < 2:
    raise SystemExit("Single and batch discard must publish a transition status")
discard_source = source[source.index("void StorageRecoveryBridge::discardSelectedRun"):]
if "analysisResult_.clear()" not in discard_source:
    raise SystemExit("Persisted discard must clear stale result")
batch_discard = source[source.index("void StorageRecoveryBridge::discardRuns"):source.index("void StorageRecoveryBridge::discardSelectedRun")]
if "analysisResult_.clear()" not in batch_discard:
    raise SystemExit("Batch persisted discard must clear stale result")
if "Refreshing persisted recovery" not in source:
    raise SystemExit("Recovery refresh must publish a transition status")
if "Persisted recovery refreshed" not in source:
    raise SystemExit("Recovery refresh must publish completion status")
if "Persisted recovery selected" not in source:
    raise SystemExit("Recovery selection must publish a lifecycle status")
if "Persisted recovery selection cleared" not in source:
    raise SystemExit("Recovery selection clearing must publish a lifecycle status")
print(f"Persisted retry action contract verified ({len(required)} checks)")
