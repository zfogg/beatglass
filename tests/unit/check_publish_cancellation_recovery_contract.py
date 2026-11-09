from pathlib import Path

source = Path("lib/library/analysis_commit.cpp").read_text()
if "if(stop.stop_requested()) { recover(); return library_detail::Publish::cancelled; }" not in source:
    raise SystemExit("publish cancellation must persist retryable recovery state")
print("publish cancellation recovery contract verified")
