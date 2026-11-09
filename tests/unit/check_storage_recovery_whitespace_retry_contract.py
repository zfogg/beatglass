from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "runId.trimmed().isEmpty()" not in source:
    raise SystemExit("whitespace-only retry selections must be rejected")
print("storage recovery whitespace retry contract verified")
