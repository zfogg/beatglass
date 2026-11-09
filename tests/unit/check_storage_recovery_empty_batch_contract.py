from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
expected = 'void StorageRecoveryBridge::discardRuns(const QStringList& ids) { if (worker_) QMetaObject::invokeMethod(worker_, "discardBatch", Qt::QueuedConnection, Q_ARG(QStringList, ids)); }'
if expected not in source:
    raise SystemExit("empty batch discard must reach the worker for actionable feedback")
print("empty batch discard feedback contract verified")
