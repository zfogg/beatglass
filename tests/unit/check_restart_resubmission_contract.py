from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8")
required = ["resubmit_retryable", "if (!storage_.retryable_run(run_id)) return {};", "return submit(std::move(run_id)"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing restart resubmission markers: " + ", ".join(missing))
print(f"Restart resubmission contract verified ({len(required)} checks)")
