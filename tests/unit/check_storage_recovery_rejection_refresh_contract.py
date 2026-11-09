from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "retry_rejection_message(rejection))); refresh(); return;" not in source:
    raise SystemExit("retry rejection must refresh persisted recovery state")
print("storage recovery rejection refresh contract verified")
