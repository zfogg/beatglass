"""Dependency-light checks for coordinator lifecycle identity/state exposure."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8")
source = (root / "lib/library/analysis_coordinator.cpp").read_text(encoding="utf-8")
required = {
    "run identity": "std::string run_id;",
    "analysis identity": "AnalysisIdentity identity;",
    "completion state": "TrackedState::completed",
    "failure state": "TrackedState::failed",
    "reserved state": "analysis_reserved{}",
    "submission preserves identity": "submitted_identity",
}
missing = [name for name, marker in required.items() if marker not in header + source]
if missing:
    raise SystemExit("Missing lifecycle model markers: " + ", ".join(missing))
print(f"Coordinator lifecycle model verified ({len(required)} checks)")
