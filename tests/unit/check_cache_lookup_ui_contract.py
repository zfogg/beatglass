from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "analysisCacheLookupControls"',
    'objectName: "analysisCacheContent"',
    'objectName: "analysisCacheEngine"',
    'objectName: "analysisCacheConfig"',
    'objectName: "analysisCacheMode"',
    'objectName: "lookupAnalysisCache"',
    'objectName: "analysisCacheResult"',
    'storageRecovery.cachedResult',
    'objectName: "analysisQueueOutcome"',
    'window.analysisJobBridge.status',
    'window.analysisJobBridge.canRetry ? " · retry available"',
    'objectName: "analysisQueueProgress"',
    'window.analysisJobBridge.frames',
    'storageRecovery.findCached(cacheContent.text, cacheEngine.text, cacheConfig.text, cacheMode.text)',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing cache lookup UI markers: " + ", ".join(missing))
print(f"Cache lookup UI contract verified ({len(required)} checks)")
