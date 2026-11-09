from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
expected = 'status_ = worker_ ? QStringLiteral("Storage closed") : QStringLiteral("Storage error: worker unavailable"); emit statusChanged();'
if expected not in source:
    raise SystemExit("close must not report success when its worker is unavailable")
print("recovery close guard contract verified")
