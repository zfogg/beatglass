from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ('std::optional<beatglass::library_detail::RetryableRun> found;', 'catch (const std::exception& error)', 'retryable_run(runId.toStdString())'):
    if marker not in source:
        raise SystemExit(f"missing retry lookup error marker: {marker}")
print("storage recovery retry lookup error contract verified")
