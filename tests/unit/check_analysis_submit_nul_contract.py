from pathlib import Path

source = Path("lib/library/analysis_coordinator.cpp").read_text()
if source.count("run_id.find('\\0') != std::string::npos") < 2:
    raise SystemExit("analysis submission paths must reject NUL-containing run IDs")
print("analysis submit NUL validation contract verified")
