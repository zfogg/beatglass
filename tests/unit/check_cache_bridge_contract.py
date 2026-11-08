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
}
haystack = header + source + qml
missing = [name for name, marker in required.items() if marker not in haystack]
if missing:
    raise SystemExit("Missing cache bridge contract markers: " + ", ".join(missing))
print(f"Desktop cache bridge contract verified ({len(required)} checks)")
