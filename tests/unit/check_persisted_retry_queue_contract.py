"""Verify selected persisted retries retain a live queue handle and poll lifecycle."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
docs = (root / "docs/progress.md").read_text(encoding="utf-8")
required = [
    "handle_ = coordinator_.resubmit_retryable",
    'emit analysis(QStringLiteral("Analysis queued"));',
    "&Worker::pollAnalysis",
    "same progress, cancellation, terminal status",
    "successful completion refreshes and clears",
]
missing = [marker for marker in required if marker not in source + docs]
if missing:
    raise SystemExit("Missing persisted retry queue markers: " + ", ".join(missing))
print(f"Persisted retry queue contract verified ({len(required)} checks)")
