from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void refresh()")
body = source[start:source.index("void discard(", start)]
for marker in ('catch (const std::exception& error)', 'Recovery refresh failed:', 'emit runsChanged({}, {});'):
    if marker not in body:
        raise SystemExit(f"missing recovery refresh error marker: {marker}")
print("storage recovery refresh error contract verified")
