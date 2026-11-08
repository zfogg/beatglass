"""Check the dependency-light Library queue/cache transition contract."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
job_bridge = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
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
}
missing = [name for name, marker in required.items() if marker not in qml + bridge + job_bridge]
if missing:
    raise SystemExit("Missing desktop analysis transition markers: " + ", ".join(missing))
if qml.index('objectName: "analysisProgress"') > qml.index('objectName: "analysisCacheStatus"'):
    raise SystemExit("cache status must follow queue progress in the Library surface")
print(f"Desktop analysis transition contract verified ({len(required)} checks)")
