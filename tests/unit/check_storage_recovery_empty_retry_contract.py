from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if 'runId.isEmpty()' not in source or 'no persisted run selected' not in source:
    raise SystemExit("empty retry selection must be rejected with actionable status")
print("storage recovery empty retry contract verified")
