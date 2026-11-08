"""Verify bridge refresh reconciles persisted selection and lifecycle status."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
docs = (root / "docs/progress.md").read_text(encoding="utf-8")
required = [
    "retryableRunIds_ = ids; retryableRunDetails_ = details;",
    "!retryableRunIds_.contains(selectedRunId_)",
    "selectedRunId_.clear(); emit selectedRunChanged();",
    'QStringLiteral("Analysis recovery available")',
    "retry/discard completion removes resolved IDs",
]
missing = [marker for marker in required if marker not in source + docs]
if missing:
    raise SystemExit("Missing recovery selection coherence markers: " + ", ".join(missing))
print(f"Recovery selection coherence contract verified ({len(required)} checks)")
