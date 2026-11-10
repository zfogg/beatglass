"""Dependency-light contract check for desktop cache lookup handoff."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (ROOT / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = {
    "QML cache properties": 'Q_PROPERTY(bool cacheHit READ cacheHit NOTIFY cachedResultChanged)',
    "QML lookup invokable": 'Q_INVOKABLE void findCached',
    "worker lookup slot": 'void findCached(QString content, QString engine, QString configuration, QString mode)',
    "queued worker dispatch": 'QMetaObject::invokeMethod(worker_, "findCached", Qt::QueuedConnection',
    "worker result signal": 'void cached(bool hit, QString result);',
    "visible cache status": 'objectName: "analysisCacheStatus"',
    "hit state binding": 'storageRecovery.cacheHit ? "Cached analysis ready"',
    "coordinator-owned worker": 'AnalysisCoordinator coordinator_',
    "coordinator storage lookup": 'coordinator_.storage().find_cached',
    "coordinator analysis submit": 'Q_INVOKABLE void submitAnalysis',
    "analysis status property": 'Q_PROPERTY(QString analysisStatus READ analysisStatus',
    "analysis result property": 'Q_PROPERTY(QString analysisResult READ analysisResult',
    "analysis cancel": 'Q_INVOKABLE void cancelAnalysis()',
    "analysis retry": 'Q_INVOKABLE void retryAnalysis()',
    "admission-aware submit": 'coordinator_.submit_admitted',
    "cache skips decoder": 'Cached analysis ready',
    "admission properties": 'Q_PROPERTY(bool analysisReserved READ analysisReserved NOTIFY analysisAdmissionChanged)',
    "admission invokable": 'Q_INVOKABLE void admitAnalysis',
    "worker admission": 'coordinator_.admit_analysis',
    "admission signal": 'void admitted(bool reserved, QString cachedResult);',
}
haystack = header + source + qml
missing = [name for name, marker in required.items() if marker not in haystack]
if missing:
    raise SystemExit("Missing cache bridge contract markers: " + ", ".join(missing))
lookup = source[source.index("void findCached("):source.index("void invalidateCached(")]
invalidate = source[source.index("void invalidateCached("):source.index("void submitAnalysis(")]
for marker in ("Cache lookup rejected:", "catch (const std::exception& error)"):
    if marker not in lookup:
        raise SystemExit(f"Missing cache lookup recovery marker: {marker}")
for marker in ("Cache invalidation rejected:", "catch (const std::exception& error)"):
    if marker not in invalidate:
        raise SystemExit(f"Missing cache invalidation recovery marker: {marker}")
if invalidate.count("emit cached(false, QString{})") < 2:
    raise SystemExit("Cache invalidation must clear stale presentation on failure and success")
if "analysisResult_.clear()" not in source[source.index('if (ok && text == QStringLiteral("Cached analysis invalidated")'):source.index("emit statusChanged();")]:
    raise SystemExit("Cache invalidation must clear the displayed analysis result")
invalidation_state = source[source.index('if (ok && text == QStringLiteral("Cached analysis invalidated")'):source.index("emit statusChanged();")]
if "analysisRetryable_ = false" not in invalidation_state or "analysisError_.clear()" not in invalidation_state:
    raise SystemExit("Cache invalidation must clear stale recovery state")
cached_connection = source[source.index("connect(worker_, &Worker::cached,"):source.index("connect(worker_, &Worker::analysis,")]
if "analysisAdmissionResult_.clear()" not in cached_connection:
    raise SystemExit("Cache lookup must clear stale admission presentation")
admitted_connection = source[source.index("connect(worker_, &Worker::admitted,"):source.index("workerThread_.start();")]
if "cacheHit_ = !result.isEmpty()" not in admitted_connection:
    raise SystemExit("Admission results must synchronize cached-result state")
if "Cached analysis ready" not in admitted_connection or "Analysis queued" not in admitted_connection:
    raise SystemExit("Admission results must expose an explicit lifecycle status")
if "analysisRetryable_ = false" not in admitted_connection or "analysisError_.clear()" not in admitted_connection:
    raise SystemExit("Admission must clear stale recovery state")
if "analysisFrames_ = 0" not in cached_connection or "analysisFrames_ = 0" not in admitted_connection:
    raise SystemExit("Cache/admission transitions must clear stale progress")
if 'Cached analysis ready' not in cached_connection or 'Cache lookup complete' not in cached_connection:
    raise SystemExit("Cache lookup must expose an explicit lifecycle status")
if "if (!hit && !analysisResult_.isEmpty())" not in cached_connection:
    raise SystemExit("Cache misses must clear stale analysis results")
if "const auto displayed = hit ? result : QString{}" not in cached_connection:
    raise SystemExit("Cache hits must publish the cached result to the analysis display")
if "analysisRetryable_ = false" not in cached_connection or "analysisError_.clear()" not in cached_connection:
    raise SystemExit("Cache lookup must clear stale recovery state")
print(f"Desktop cache bridge contract verified ({len(required)} checks)")
