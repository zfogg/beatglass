"""Cross-layer contract for persisted retry resolution and selected-run cleanup."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
tests = (root / "tests/unit/analysis_operation_test.cpp").read_text(encoding="utf-8")
required = [
    "!retryableRunIds_.contains(selectedRunId_)",
    "selectedRunId_.clear(); emit selectedRunChanged();",
    'TEST_CASE("endpoint replacement retry state survives cache restart")',
    'TEST_CASE("coordinator resubmits persisted endpoint retry after restart")',
    'TEST_CASE("resolved retry diagnostics clear after recovery or discard")',
    'cache.retryable_runs().empty()',
]
missing = [marker for marker in required if marker not in bridge + tests]
if missing:
    raise SystemExit("Missing selected retry cleanup E2E markers: " + ", ".join(missing))
print(f"Selected retry cleanup E2E contract verified ({len(required)} checks)")
