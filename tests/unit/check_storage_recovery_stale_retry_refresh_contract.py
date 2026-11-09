from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "missing_source_snapshot" not in source or "missing_source_snapshot))); refresh(); return;" not in source:
    raise SystemExit("stale persisted retries must refresh selected recovery state")
print("storage recovery stale retry refresh contract verified")
