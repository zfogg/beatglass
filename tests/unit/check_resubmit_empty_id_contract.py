from pathlib import Path

source = Path("include/beatglass/library/analysis_coordinator.hpp").read_text()
if "if (run_id.empty()) return {};" not in source:
    raise SystemExit("retry resubmission must reject an empty ID before storage lookup")
print("empty retry resubmission guard verified")
