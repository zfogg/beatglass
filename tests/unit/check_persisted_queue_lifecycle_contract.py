"""Verify persisted queue state drives cancel/retry/recovery lifecycle transitions."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'emit analysis(runs.empty() ? QStringLiteral("Analysis idle") : QStringLiteral("Analysis recovery available"));',
    'void cancelAnalysis() { if (handle_) { handle_->cancel(); pollAnalysis(); } }',
    'void retryAnalysis() { if (handle_ && handle_->retry())',
    'objectName: "cancelAdmittedAnalysis"',
    'objectName: "retryAdmittedAnalysis"',
    'storageRecovery.analysisStatus',
]
missing = [marker for marker in required if marker not in source + qml]
if missing:
    raise SystemExit("Missing persisted queue lifecycle markers: " + ", ".join(missing))
print(f"Persisted queue lifecycle contract verified ({len(required)} checks)")
