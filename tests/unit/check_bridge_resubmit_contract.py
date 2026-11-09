from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = "".join((root / p).read_text(encoding="utf-8") for p in ("src/desktop/storage_recovery_bridge.hpp", "src/desktop/storage_recovery_bridge.cpp", "src/desktop/qml/Main.qml"))
required = ["retrySelectedRun()", "retryable_runs()", "found->source_path", "found->decoder_snapshot", "retry_rejection_message", "RetryRejectionReason::incompatible_decoder_snapshot", "RetryRejectionReason::stale_source_identity", "resubmit_retryable", "make_file_decode_operation"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing bridge resubmit markers: " + ", ".join(missing))
print(f"Bridge resubmit contract verified ({len(required)} checks)")
