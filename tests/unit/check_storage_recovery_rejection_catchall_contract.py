from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "Retry rejected: unknown storage error" not in source:
    raise SystemExit("unknown retry rejection errors must be surfaced")
print("storage recovery rejection catch-all contract verified")
