from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for name in ("findCached", "invalidateCached"):
    if f'void StorageRecoveryBridge::{name}' not in source:
        raise SystemExit(f"missing {name} bridge action")
    if f'else {{ status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); }}' not in source[source.index(f"void StorageRecoveryBridge::{name}"):]:
        raise SystemExit(f"{name} must report an unavailable worker")
print("recovery cache action contracts verified")
