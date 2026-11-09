from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void open(QString path)")
body = source[start:source.index("void close()", start)]
if 'diagnostic = "unknown storage error";' not in body or "catch (...)" not in body:
    raise SystemExit("unknown storage open errors must become diagnostics")
print("storage recovery open unknown error contract verified")
