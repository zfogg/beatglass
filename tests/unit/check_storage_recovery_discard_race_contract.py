from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ('QStringLiteral("Retryable run is active")', 'TrackedState::queued', 'TrackedState::running'):
    if marker not in source:
        raise SystemExit(f"missing active discard race guard: {marker}")
print("storage recovery discard race contract verified")
