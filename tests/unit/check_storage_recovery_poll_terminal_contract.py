from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "if (state == TrackedState::completed) handle_.reset();" not in source:
    raise SystemExit("completed analysis must release the worker handle after refresh")
print("storage recovery poll terminal contract verified")
