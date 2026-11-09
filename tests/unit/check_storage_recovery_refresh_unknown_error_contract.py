from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void refresh()")
body = source[start:source.index("void discard(", start)]
if "Recovery refresh failed: unknown storage error" not in body or "catch (...)" not in body:
    raise SystemExit("unknown refresh errors must clear visible recovery state")
print("storage recovery refresh unknown error contract verified")
