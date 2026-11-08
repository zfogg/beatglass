"""Verify persisted retryable runs restore an explicit analysis lifecycle status."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'emit analysis(runs.empty() ? QStringLiteral("Analysis idle") : QStringLiteral("Analysis recovery available"));',
    'objectName: "analysisAdmissionStatus"',
    'storageRecovery.analysisStatus',
    'objectName: "persistedRecoveryStatus"',
]
missing = [marker for marker in required if marker not in source + qml]
if missing:
    raise SystemExit("Missing persisted analysis status markers: " + ", ".join(missing))
print(f"Persisted analysis status contract verified ({len(required)} checks)")
