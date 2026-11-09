from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void retry(QString runId)")
body = source[start:source.index("void findCached", start)]
if 'if (handle_) { refresh(); emit analysis(QStringLiteral("Analysis queued"));' not in body:
    raise SystemExit("successful persisted retry must refresh recovery list before queueing analysis")
print("storage recovery retry refresh contract verified")
