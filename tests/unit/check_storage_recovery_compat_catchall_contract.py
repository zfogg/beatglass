from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "Retry rejected: unknown decoder error" not in source:
    raise SystemExit("unknown decoder compatibility errors must be surfaced")
print("storage recovery compatibility catch-all contract verified")
