from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void discard(QString id)")
body = source[start:source.index("void discardBatch", start)]
if "catch (...)" not in body or "unknown storage error" not in body:
    raise SystemExit("unknown single discard errors must be surfaced")
print("storage recovery single unknown error contract verified")
