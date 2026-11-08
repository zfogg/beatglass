from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8")
required = ["retry_rejection_reason(run_id, operation)", "RetryRejectionReason::none", "stale_source_identity", "incompatible_decoder_snapshot"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing retry identity guard markers: " + ", ".join(missing))
print(f"Retry identity guard contract verified ({len(required)} checks)")
