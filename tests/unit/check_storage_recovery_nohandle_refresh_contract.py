from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "if (!handle_) refresh();" not in source:
    raise SystemExit("failed retry admission must refresh persisted recovery state")
print("storage recovery no-handle refresh contract verified")
