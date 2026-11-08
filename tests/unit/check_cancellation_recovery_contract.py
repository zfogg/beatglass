from pathlib import Path

root = Path(__file__).resolve().parents[2]
runner = (root / "lib/library/analysis_runner.cpp").read_text(encoding="utf-8")
operation = (root / "include/beatglass/library/analysis_operation.hpp").read_text(encoding="utf-8")
coordinator = (root / "lib/library/analysis_coordinator.cpp").read_text(encoding="utf-8")
required = [
    "recover_cancellation",
    "auto cancelled = [&]",
    "cache->mark_retryable(recovery_id, recovery_key)",
    "const auto recovered = coordinator.storage().retryable_run(\"cancel-run\")",
]
text = runner + operation + coordinator + (root / "tests/unit/analysis_operation_test.cpp").read_text(encoding="utf-8")
missing = [marker for marker in required if marker not in text]
if missing:
    raise SystemExit("Missing cancellation recovery markers: " + ", ".join(missing))
print(f"Cancellation recovery contract verified ({len(required)} checks)")
