from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void open(QString path)")
body = source[start:source.index("void close()", start)]
for marker in ('try { ok = coordinator_.open_storage', 'catch (const std::exception& error)', 'diagnostic = error.what();'):
    if marker not in body:
        raise SystemExit(f"missing storage open error marker: {marker}")
print("storage recovery open error contract verified")
