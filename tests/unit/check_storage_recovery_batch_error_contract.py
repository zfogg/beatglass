from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void discardBatch")
body = source[start:source.index("void retry(", start)]
for marker in ("try {", "catch (const std::exception& error)", 'Recovery discard rejected:'):
    if marker not in body:
        raise SystemExit(f"missing batch discard error handling marker: {marker}")
print("storage recovery batch error contract verified")
