from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "std::find(values.begin(), values.end(), value) == values.end()" not in source:
    raise SystemExit("batch recovery IDs must be deduplicated before storage deletion")
print("storage recovery batch dedupe contract verified")
