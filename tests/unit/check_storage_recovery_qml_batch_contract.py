from pathlib import Path

header = Path("src/desktop/storage_recovery_bridge.hpp").read_text()
source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ("discardRuns(const QStringList& ids)", '"discardBatch"', "discard_retryable(values)"):
    if marker not in header + source:
        raise SystemExit(f"missing batch recovery bridge marker: {marker}")
print("storage recovery QML batch bridge contract verified")
