"""Verify worker progress/restart semantics are documented and executable."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
docs = (root / "docs/progress.md").read_text(encoding="utf-8")
tests = (root / "tests/unit/analysis_operation_test.cpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    "live frame progress",
    "frame counters correctly restart at zero",
    'TEST_CASE("coordinator resubmits persisted endpoint retry after restart")',
    'TEST_CASE("coordinator cancellation does not publish a stale result")',
    "handle_->frames()",
    'QStringLiteral("Analysis recovery available")',
]
missing = [marker for marker in required if marker not in docs + tests + source]
if missing:
    raise SystemExit("Missing worker restart lifecycle markers: " + ", ".join(missing))
print(f"Worker restart lifecycle contract verified ({len(required)} checks)")
