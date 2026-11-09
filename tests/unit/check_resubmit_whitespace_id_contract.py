from pathlib import Path

source = Path("include/beatglass/library/analysis_coordinator.hpp").read_text()
expected = 'if (run_id.empty() || run_id.find_first_not_of(" \\t\\r\\n") == std::string::npos) return {};'
if expected not in source:
    raise SystemExit("retry resubmission must reject whitespace-only IDs")
print("whitespace retry resubmission guard verified")
