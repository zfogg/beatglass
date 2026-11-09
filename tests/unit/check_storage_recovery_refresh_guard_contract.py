from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
expected = 'void StorageRecoveryBridge::refreshRetryableRuns() { if (worker_) QMetaObject::invokeMethod(worker_, "refresh", Qt::QueuedConnection); else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); } }'
if expected not in source:
    raise SystemExit("manual recovery refresh must report an unavailable worker")
print("recovery refresh worker guard contract verified")
