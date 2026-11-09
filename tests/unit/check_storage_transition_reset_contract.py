"""Verify open/failure transitions cannot retain stale persisted recovery UI state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    "if (!ok) {",
    "if (!retryableRunIds_.isEmpty()) { retryableRunIds_.clear(); emit retryableRunsChanged(); }",
    "if (!selectedRunId_.isEmpty()) { selectedRunId_.clear(); emit selectedRunChanged(); }",
    "if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }",
    "if (!resetStatus_.isEmpty()) { resetStatus_.clear(); emit resetStatusChanged(); }",
    'analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();',
    'status_ = QStringLiteral("Opening storage"); emit statusChanged();',
]
missing = [marker for marker in required if marker not in source]
if missing:
    raise SystemExit("Missing storage transition reset markers: " + ", ".join(missing))
print(f"Storage transition reset contract verified ({len(required)} checks)")
