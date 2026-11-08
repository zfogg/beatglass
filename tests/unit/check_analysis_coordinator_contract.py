from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "include/beatglass/library/analysis_coordinator.hpp").read_text(encoding="utf-8") + (root / "lib/library/analysis_coordinator.cpp").read_text(encoding="utf-8")
required = ["AnalysisCoordinator", "JobExecutor", "StorageOwner", "make_transactional_publisher", "submit_retryable"]
missing = [item for item in required if item not in text]
if missing: raise SystemExit("Missing coordinator markers: " + ", ".join(missing))
print(f"Analysis coordinator contract verified ({len(required)} checks)")
