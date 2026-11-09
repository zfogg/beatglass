from pathlib import Path

source = Path("lib/library/analysis_coordinator.cpp").read_text()
expected = 'run_id.find_first_not_of(" \\t\\r\\n") == std::string::npos'
if expected not in source:
    raise SystemExit("analysis submit must reject whitespace-only run IDs")
print("analysis submit ID validation contract verified")
