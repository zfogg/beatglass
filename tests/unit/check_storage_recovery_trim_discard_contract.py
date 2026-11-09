from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "selectedRunId_ = selectedRunId_.trimmed();" not in source:
    raise SystemExit("discard selection must be normalized before storage deletion")
print("storage recovery trim discard contract verified")
