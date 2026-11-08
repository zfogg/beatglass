from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void discardBatch")
body = source[start:source.index("void retry(", start)]
for marker in ("discard_retryable(values)", "emit result(complete", "if (removed) refresh();"):
    if marker not in body:
        raise SystemExit(f"batch discard must refresh visible state: {marker}")
print("storage recovery batch refresh contract verified")
