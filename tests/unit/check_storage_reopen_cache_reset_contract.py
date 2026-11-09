"""Contract checks for clearing cached lookup state before storage reopen."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
SOURCE = ROOT / "src" / "desktop" / "storage_recovery_bridge.cpp"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    open_body = source.split("void StorageRecoveryBridge::open()", 1)[1].split("void StorageRecoveryBridge::close()", 1)[0]
    required = [
        'if (cacheHit_) { cacheHit_ = false; emit cachedResultChanged(); }',
        'if (!cachedResult_.isEmpty()) { cachedResult_.clear(); emit cachedResultChanged(); }',
        'QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection',
        'status_ = QStringLiteral("Opening storage"); emit statusChanged();',
    ]
    missing = [marker for marker in required if marker not in open_body]
    if missing:
        raise AssertionError("missing storage reopen cache reset markers: " + ", ".join(missing))
    if "Cached lookup state is reset at the same boundary" not in doc:
        raise AssertionError("missing storage reopen cache reset documentation")
    print(f"storage reopen cache reset contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
