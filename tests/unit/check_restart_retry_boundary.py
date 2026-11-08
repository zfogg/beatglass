from pathlib import Path
root = Path(__file__).resolve().parents[2]
header = (root / "include/beatglass/library/storage_owner.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = ["retryable_run(const std::string& id)", "source/decoder metadata", "coordinator_.storage().retryable_run", "missing_source_snapshot", "incompatible_decoder_snapshot"]
missing = [m for m in required if m not in header + source]
if missing: raise SystemExit("Missing restart retry markers: " + ", ".join(missing))
print(f"Restart retry boundary verified ({len(required)} checks)")
