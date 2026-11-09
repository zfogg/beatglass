from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
required = [
    "Q_INVOKABLE void discard();",
    "Q_INVOKABLE void discardBatch();",
    "retryable_.reset();",
    "batchAccepted_ = batchRejected_ = batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0;",
    'status_ = QStringLiteral(\"Idle\")',
]
missing = [marker for marker in required if marker not in header + source]
if missing:
    raise SystemExit("Missing bridge recovery reset markers: " + ", ".join(missing))
print(f"Bridge recovery reset contract verified ({len(required)} checks)")
