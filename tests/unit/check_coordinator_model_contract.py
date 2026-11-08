"""Dependency-light contract check for the Library-facing admission model."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8")
bridge = (ROOT / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = {
    "coordinator admission API": "CacheAdmission admit_analysis(const std::string& run_id",
    "bridge uses model": "coordinator_.admit_analysis(runId.toStdString(), identity)",
    "submission state": "struct Submission",
    "cache hit state": "std::optional<std::string> cached",
    "reserved state": "bool analysis_reserved{}",
}
missing = [name for name, marker in required.items() if marker not in header + bridge]
if missing:
    raise SystemExit("Missing coordinator model markers: " + ", ".join(missing))
print(f"Coordinator model contract verified ({len(required)} checks)")
