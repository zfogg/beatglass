"""Check the dependency-light Library queue/cache transition contract."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
job_bridge = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
job_bridge_header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
required = {
    "queue progress": 'objectName: "analysisProgress"',
    "queue status": 'analysisJobBridge.status',
    "retry transition": 'objectName: "retryAnalysisBatch"',
    "cancel transition": 'objectName: "cancelAnalysisBatch"',
    "cache status": 'objectName: "analysisCacheStatus"',
    "cache hit state": 'storageRecovery.cacheHit ? "Cached analysis ready"',
    "async cache lookup": 'Q_INVOKABLE void findCached',
    "lifetime-safe progress callback": 'weak_self = weak_self_',
    "guarded queued update": 'if (!weak_self) return;',
    "worker result property": 'Q_PROPERTY(QString result READ result',
    "worker result handoff": 'op.publish = [weak_self = weak_self_, publish]',
    "worker result rendering": 'objectName: "analysisWorkerResult"',
    "retry clears stale result": 'if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }',
    "batch result count": 'Q_PROPERTY(int batchResultsPublished READ batchResultsPublished',
    "batch result handoff": '++weak_self->batchResultsPublished_',
    "batch retry resets result count": 'batchResultsPublished_ = 0;',
    "fresh batch clears single result": 'if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }',
    "batch latest result": 'Q_PROPERTY(QString batchLastResult READ batchLastResult',
    "batch result rendering": 'objectName: "analysisBatchLastResult"',
    "bounded displayed result": 'kMaxDisplayedResultBytes = 4096',
    "fresh batch resets frames": 'if (frames_ != 0) { frames_ = 0; emit progressChanged(); }',
    "single retry emits progress": 'frames_ = retryable_->frames();\n    emit progressChanged();',
    "invalid submit clears frames": 'if (frames_ != 0) { frames_ = 0; emit progressChanged(); }',
    "source attribution property": 'Q_PROPERTY(QString sourcePath READ sourcePath',
    "source attribution rendering": 'objectName: "analysisSourcePath"',
}
missing = [name for name, marker in required.items() if marker not in qml + bridge + job_bridge + job_bridge_header]
if missing:
    raise SystemExit("Missing desktop analysis transition markers: " + ", ".join(missing))
if qml.index('objectName: "analysisProgress"') > qml.index('objectName: "analysisCacheStatus"'):
    raise SystemExit("cache status must follow queue progress in the Library surface")
print(f"Desktop analysis transition contract verified ({len(required)} checks)")
