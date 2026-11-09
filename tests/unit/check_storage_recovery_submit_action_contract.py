from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
expected = 'if (worker_) QMetaObject::invokeMethod(worker_, "submitAnalysis", Qt::QueuedConnection, Q_ARG(QString, runId), Q_ARG(QString, path), Q_ARG(QString, contentHash), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode)); else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); }'
if expected not in source:
    raise SystemExit("analysis submission must report an unavailable worker")
print("recovery submit action contract verified")
