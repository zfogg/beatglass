from pathlib import Path

source = Path("include/beatglass/library/analysis_coordinator.hpp").read_text()
expected = 'if (run_id.empty() || run_id.find_first_not_of(" \\t\\r\\n") == std::string::npos) return RetryRejectionReason::missing_source_snapshot;'
if expected not in source:
    raise SystemExit("retry rejection classification must handle blank IDs without storage exceptions")
print("blank retry rejection classification verified")
