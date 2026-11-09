from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
expected = 'void StorageRecoveryBridge::retrySelectedRun() { selectedRunId_ = selectedRunId_.trimmed(); if (!selectedRunId_.isEmpty()) QMetaObject::invokeMethod(worker_, "retry", Qt::QueuedConnection, Q_ARG(QString, selectedRunId_)); }'
if expected not in source:
    raise SystemExit("retry selection must normalize the selected persisted-run ID")
print("retry selection normalization contract verified")
