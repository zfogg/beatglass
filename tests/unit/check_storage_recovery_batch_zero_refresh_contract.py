from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void discardBatch")
body = source[start:source.index("void retry(", start)]
if "emit result(complete" not in body or "refresh();" not in body:
    raise SystemExit("batch discard must report and refresh even when no rows are removed")
print("storage recovery zero-result refresh contract verified")
