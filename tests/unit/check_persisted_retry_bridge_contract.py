from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = ["retryable_run(runId.toStdString())", "file_decoder_snapshot_compatible", "make_file_decode_operation(found->source_path", "resubmit_retryable"]
missing = [m for m in required if m not in source]
if missing: raise SystemExit("Missing persisted retry bridge markers: " + ", ".join(missing))
print(f"Persisted retry bridge contract verified ({len(required)} checks)")
