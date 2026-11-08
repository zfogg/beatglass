from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ('QStringLiteral("Retryable run is active")', 'return; } const bool ok = coordinator_.storage().discard_retryable', 'if (ok) { handle_.reset();'):
    if marker not in source:
        raise SystemExit(f"missing active discard result lifecycle marker: {marker}")
print("storage recovery active discard result contract verified")
