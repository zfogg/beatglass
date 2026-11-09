from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("  void discard(QString id)")
end = source.index("  void discardBatch", start)
body = source[start:end]
if "if (ok) { handle_.reset(); emit analysis(QStringLiteral(\"Analysis idle\")); } refresh();" not in body:
    raise SystemExit("single discard must refresh even when the persisted run is not found")
print("single discard refresh contract verified")
