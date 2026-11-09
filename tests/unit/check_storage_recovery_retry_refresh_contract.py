from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
required = (
    'if (runId.isEmpty()) { emit result(false, QStringLiteral("Retry rejected: no persisted run selected")); refresh(); return; }',
    'catch (...) { emit result(false, QStringLiteral("Retry rejected: unknown storage error")); refresh(); return; }',
    'catch (const std::exception& error) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromUtf8(error.what())); refresh(); return; }',
)
for fragment in required:
    if fragment not in source:
        raise SystemExit(f"missing retry refresh contract: {fragment}")
print("retry rejection refresh contracts verified")
