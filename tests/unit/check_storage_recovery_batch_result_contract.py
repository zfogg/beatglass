from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ("const bool complete = removed == values.size()", "of %2 retryable recovery runs discarded", "emit result(complete"):
    if marker not in source:
        raise SystemExit(f"missing batch result marker: {marker}")
print("storage recovery batch result contract verified")
