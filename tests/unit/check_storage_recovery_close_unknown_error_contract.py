from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void close()")
body = source[start:source.index("void refresh()", start)]
if 'Storage close failed: unknown storage error' not in body or "catch (...)" not in body:
    raise SystemExit("unknown storage close errors must be surfaced")
print("storage recovery close unknown error contract verified")
