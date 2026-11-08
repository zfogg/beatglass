"""Verify closing storage clears transient recovery state in the desktop bridge."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    'void StorageRecoveryBridge::close() {',
    "retryableRunIds_.clear(); emit retryableRunsChanged();",
    "selectedRunId_.clear(); emit selectedRunChanged();",
    "analysisError_.clear(); emit analysisErrorChanged();",
    "resetStatus_.clear(); emit resetStatusChanged();",
    "analysisRetryable_ = false; emit analysisRetryableChanged();",
    "analysisFrames_ = 0; emit analysisProgressChanged();",
    'status_ = QStringLiteral("Storage closed"); emit statusChanged();',
]
missing = [marker for marker in required if marker not in source]
if missing:
    raise SystemExit("Missing storage-close reset markers: " + ", ".join(missing))
print(f"Storage close reset contract verified ({len(required)} checks)")
