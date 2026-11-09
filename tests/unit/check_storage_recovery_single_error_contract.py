from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void discard(QString id)")
body = source[start:source.index("void discardBatch", start)]
for marker in ("try {", "catch (const std::exception& error)", 'Recovery discard rejected:'):
    if marker not in body:
        raise SystemExit(f"missing single discard error handling marker: {marker}")
print("storage recovery single discard error contract verified")
