from pathlib import Path

source = Path("lib/library/analysis_coordinator.cpp").read_text()
if source.count("run_id.size() > 4096") < 2:
    raise SystemExit("analysis submission paths must reject overlong run IDs before storage")
print("analysis submit length validation contract verified")
