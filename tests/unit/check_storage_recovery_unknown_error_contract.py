from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if 'catch (...) { emit result(false, QStringLiteral("Recovery discard rejected: unknown storage error"))' not in source:
    raise SystemExit("unknown recovery discard errors must be surfaced")
print("storage recovery unknown error contract verified")
