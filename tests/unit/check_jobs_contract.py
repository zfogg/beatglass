from pathlib import Path

source = (Path(__file__).parents[2] / "include/beatglass/library/jobs.hpp").read_text(encoding="utf-8")
for marker in ("constexpr bool isTerminal", "constexpr bool hasKnownTotal", "constexpr bool canTransition", "constexpr bool isValidProgress", "JobStatus::cancelled", "total_frames > 0"):
    if marker not in source:
        raise SystemExit(f"Missing jobs contract marker: {marker}")
print("jobs lifecycle contract verified")
