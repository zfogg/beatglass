from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "emit result(state == TrackedState::completed, QString::fromLatin1(status));" not in source:
    raise SystemExit("terminal recovery result must only report success for completion")
print("storage recovery terminal result contract verified")
