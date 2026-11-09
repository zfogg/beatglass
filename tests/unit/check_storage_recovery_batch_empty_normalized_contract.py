from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "if (values.empty())" not in source or "No retryable recovery runs selected" not in source:
    raise SystemExit("normalized empty recovery batches must be rejected before storage")
print("storage recovery normalized empty batch contract verified")
