from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
required = [
    'Analysis cancelled before publication; retry is available.',
    'Worker analysis failed before publishing a verified result; retry is available.',
    'Analysis batch cancelled before publication; retry is available.',
    'One or more batch operations failed before publishing a verified result.',
    'failureReason_ += QStringLiteral(" Retry is available.")',
    'Q_PROPERTY(bool canRetry READ canRetry NOTIFY statusChanged)',
    'Q_INVOKABLE void retry();',
]
missing = [marker for marker in required if marker not in source + header]
if missing:
    raise SystemExit("Missing bridge recovery action markers: " + ", ".join(missing))
print(f"Bridge recovery action contract verified ({len(required)} checks)")
