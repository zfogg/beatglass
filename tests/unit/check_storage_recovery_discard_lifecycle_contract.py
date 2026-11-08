from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ('handle_.reset(); emit analysis(QStringLiteral("Analysis idle")); refresh();', 'if (removed) { handle_.reset();'):
    if marker not in source:
        raise SystemExit(f"missing discard lifecycle marker: {marker}")
print("storage recovery discard lifecycle contract verified")
