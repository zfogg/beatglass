from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void close()")
body = source[start:source.index("void refresh()", start)]
for marker in ('catch (const std::exception& error)', 'Storage close failed:', 'emit runsChanged({}, {});'):
    if marker not in body:
        raise SystemExit(f"missing storage close error marker: {marker}")
print("storage recovery close error contract verified")
