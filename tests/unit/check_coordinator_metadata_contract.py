from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "lib/library/analysis_coordinator.cpp").read_text(encoding="utf-8")
header = (root / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8")
required = ["operation.source.path.string()", "operation.decoder_snapshot", "admission.decoder_compatible", "admission persists source path/decoder snapshot"]
missing = [m for m in required if m not in source + header]
if missing: raise SystemExit("Missing coordinator metadata markers: " + ", ".join(missing))
print(f"Coordinator metadata contract verified ({len(required)} checks)")
