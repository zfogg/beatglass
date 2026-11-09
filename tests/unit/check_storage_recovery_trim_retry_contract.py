from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "runId = runId.trimmed();" not in source:
    raise SystemExit("retry IDs must be normalized before persisted lookup")
print("storage recovery trim retry contract verified")
