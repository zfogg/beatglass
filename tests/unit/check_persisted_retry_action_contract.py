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
    "dedicated QThread",
]
missing = [marker for marker in required if marker not in qml + bridge + (root / "docs/library-recovery-bridge-contract.md").read_text(encoding="utf-8")]
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
if "analysisRetryable_ = false" not in batch_discard:
    raise SystemExit("Batch persisted discard must clear stale recovery state")
if "Refreshing persisted recovery" not in source:
    raise SystemExit("Recovery refresh must publish a transition status")
if "Retryable run discarded" not in source or "retryable recovery runs discarded" not in source or 'analysisStatus_ = QStringLiteral("Analysis idle")' not in source:
    raise SystemExit("Batch discard completion must return analysis state to idle")
if "Persisted recovery refreshed" not in source:
    raise SystemExit("Recovery refresh must publish completion status")
if "Persisted recovery selected" not in source:
    raise SystemExit("Recovery selection must publish a lifecycle status")
if "Discard rejected: retryable run is active" not in source:
    raise SystemExit("Active retryable jobs must reject destructive discard explicitly")
if "Persisted recovery selection cleared" not in source:
    raise SystemExit("Recovery selection clearing must publish a lifecycle status")
print(f"Persisted retry action contract verified ({len(required)} checks)")
if "Q_DISABLE_COPY(StorageRecoveryBridge)" not in bridge:
    raise SystemExit("Thread-owning recovery bridge must not be copyable")
if "Q_PROPERTY(bool storageConfigured READ storageConfigured NOTIFY databasePathChanged)" not in bridge or "bool storageConfigured() const" not in bridge:
    raise SystemExit("Recovery bridge must expose configured-storage state")
if "Q_PROPERTY(bool storageOpen READ storageOpen NOTIFY storageStateChanged)" not in bridge or "bool storageOpen() const" not in bridge:
    raise SystemExit("Recovery bridge must expose open-storage state")
if "Q_PROPERTY(bool storageTransitionPending READ storageTransitionPending NOTIFY storageStateChanged)" not in bridge:
    raise SystemExit("Recovery bridge must expose storage transition state")
if "storageTransitionPending_" not in bridge or "storageTransitionPending_ = true" not in source:
    raise SystemExit("Open-state API must distinguish transition failures")
if "const auto normalized = path.trimmed()" not in source:
    raise SystemExit("Database paths must be normalized before configuration")
if "Storage already open" not in source:
    raise SystemExit("Repeated open must be rejected explicitly")
if "Storage already closed" not in source:
    raise SystemExit("Repeated close must be rejected explicitly")
