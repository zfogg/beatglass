from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void retry(QString runId)")
body = source[start:source.index("void findCached", start)]
if body.count("Retry rejected: ") < 4 or body.count("refresh(); return;") < 4:
    raise SystemExit("retry rejection paths must refresh persisted recovery state")
print("storage recovery operation refresh contract verified")
