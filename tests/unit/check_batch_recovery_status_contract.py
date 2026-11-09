"""Verify batch status includes and clears persisted recovery diagnostics."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    'objectName: "analysisBatchStatus"',
    '" · recovery: " + storageRecovery.analysisError',
    "analysisError_.clear(); emit analysisErrorChanged();",
    'QStringLiteral("Retry queued")',
    'QStringLiteral("Retryable run discarded")',
]
missing = [marker for marker in required if marker not in qml + source]
if missing:
    raise SystemExit("Missing batch recovery status markers: " + ", ".join(missing))
print(f"Batch recovery status contract verified ({len(required)} checks)")
