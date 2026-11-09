from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
start = source.index("void retry(QString runId)")
body = source[start:source.index("void findCached", start)]
if "catch (const std::exception& error)" not in body or "Retry rejected: " not in body:
    raise SystemExit("retry admission exceptions must become actionable bridge results")
print("storage recovery retry error contract verified")
