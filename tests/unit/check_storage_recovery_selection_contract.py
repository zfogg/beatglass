from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
expected = 'void StorageRecoveryBridge::selectRun(const QString& id) { const auto normalized = id.trimmed(); if (selectedRunId_ == normalized) return; selectedRunId_ = normalized; emit selectedRunChanged(); }'
if expected not in source:
    raise SystemExit("selected recovery IDs must be normalized at the bridge boundary")
print("recovery selection normalization contract verified")
