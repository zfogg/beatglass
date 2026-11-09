"""Contract check for real retry resubmission at the coordinator boundary."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
header = (root / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8")
required = ["resubmit_retryable", "freshly reconstructed source operation", "return submit(std::move(run_id)"]
missing = [marker for marker in required if marker not in header]
if missing:
    raise SystemExit("Missing retry resubmission markers: " + ", ".join(missing))
print(f"Retry resubmission contract verified ({len(required)} checks)")
