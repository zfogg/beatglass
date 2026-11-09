from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if 'ids.isEmpty()' not in source or 'No retryable recovery runs selected' not in source:
    raise SystemExit("empty recovery batch must be rejected with actionable status")
print("storage recovery empty batch contract verified")
