from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8")
required = ["storage_.retryable_run(run_id)", "missing_source_snapshot", "stale_source_identity", "incompatible_decoder_snapshot"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing retry rejection lookup markers: " + ", ".join(missing))
if "storage_.retryable_runs()" in text: raise SystemExit("rejection path must use ID lookup")
print(f"Retry rejection lookup contract verified ({len(required)} checks)")
