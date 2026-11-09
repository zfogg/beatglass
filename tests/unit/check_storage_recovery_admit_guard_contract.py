from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if 'QMetaObject::invokeMethod(worker_, "admitAnalysis"' not in source or 'Q_ARG(QString, mode)); else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); }' not in source:
    raise SystemExit("analysis admission must report an unavailable worker")
print("recovery admission guard contract verified")
