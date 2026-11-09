from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ('RetryRejectionReason rejection;', 'retry_rejection_reason(runId.toStdString(), operation)', 'catch (const std::exception& error)'):
    if marker not in source:
        raise SystemExit(f"missing retry rejection error marker: {marker}")
print("storage recovery rejection error contract verified")
