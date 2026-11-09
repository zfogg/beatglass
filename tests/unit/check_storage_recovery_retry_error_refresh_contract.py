from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if 'handle_.reset(); emit result(false, QStringLiteral("Retry rejected: ") + QString::fromUtf8(error.what())); refresh(); return;' not in source:
    raise SystemExit("retry admission errors must refresh persisted recovery state")
print("storage recovery retry error refresh contract verified")
