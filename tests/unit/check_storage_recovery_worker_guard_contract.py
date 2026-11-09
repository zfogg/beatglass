from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for fragment in (
    'void StorageRecoveryBridge::discardSelectedRun() { selectedRunId_ = selectedRunId_.trimmed(); if (worker_ && !selectedRunId_.isEmpty())',
    'void StorageRecoveryBridge::retrySelectedRun() { selectedRunId_ = selectedRunId_.trimmed(); if (worker_ && !selectedRunId_.isEmpty())',
):
    if fragment not in source:
        raise SystemExit(f"missing worker guard: {fragment}")
print("recovery worker guard contracts verified")
