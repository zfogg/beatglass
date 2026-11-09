from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if 'if (!worker_) { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); return; }' not in source:
    raise SystemExit("open must report an unavailable worker instead of invoking a null target")
print("recovery open worker guard contract verified")
