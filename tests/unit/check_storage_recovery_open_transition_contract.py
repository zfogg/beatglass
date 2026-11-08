from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void open(QString path)")
body = source[start:source.index("void close()", start)]
for marker in ("emit runsChanged({}, {});", 'emit analysis(QStringLiteral("Analysis idle"));'):
    if marker not in body:
        raise SystemExit(f"missing pre-open recovery reset marker: {marker}")
print("storage recovery open transition contract verified")
