from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for name in ("cancelAnalysis", "retryAnalysis"):
    expected = f'void StorageRecoveryBridge::{name}() {{ if (worker_) QMetaObject::invokeMethod(worker_, "{name}", Qt::QueuedConnection); else {{ status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); }} }}'
    if expected not in source:
        raise SystemExit(f"{name} must report an unavailable worker")
print("recovery analysis action contracts verified")
