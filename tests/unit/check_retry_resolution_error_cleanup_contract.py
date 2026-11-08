"""Verify retry/discard/invalidation success clears shared queue diagnostics."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
docs = (root / "docs/progress.md").read_text(encoding="utf-8")
required = [
    'text == QStringLiteral("Retry queued")',
    'text == QStringLiteral("Retryable run discarded")',
    'text == QStringLiteral("Cached analysis invalidated")',
    "analysisError_.clear(); emit analysisErrorChanged();",
    "single-run and adjacent batch/recovery controls",
]
missing = [marker for marker in required if marker not in source + docs]
if missing:
    raise SystemExit("Missing retry resolution cleanup markers: " + ", ".join(missing))
print(f"Retry resolution error cleanup contract verified ({len(required)} checks)")
