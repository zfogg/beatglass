"""Verify changing storage path clears cache and admission presentation."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    "void StorageRecoveryBridge::setDatabasePath(const QString& path)",
    "cacheHit_ = false; emit cachedResultChanged();",
    "cachedResult_.clear(); emit cachedResultChanged();",
    "analysisResult_.clear(); emit analysisResultChanged();",
    "analysisReserved_ = false; emit analysisAdmissionChanged();",
    "analysisAdmissionResult_.clear(); emit analysisAdmissionChanged();",
    'analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();',
]
missing = [marker for marker in required if marker not in source]
if missing:
    raise SystemExit("Missing storage path switch reset markers: " + ", ".join(missing))
print(f"Storage path switch reset contract verified ({len(required)} checks)")
