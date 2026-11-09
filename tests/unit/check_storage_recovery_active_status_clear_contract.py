from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ('if (state != TrackedState::queued && state != TrackedState::running)', 'emit result(true, QString::fromLatin1(status));'):
    if marker not in source:
        raise SystemExit(f"missing terminal active-status clear marker: {marker}")
print("storage recovery active status clear contract verified")
