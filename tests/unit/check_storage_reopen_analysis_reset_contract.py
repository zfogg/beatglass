"""Contract checks for active analysis reset before storage reopen."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
SOURCE = ROOT / "src" / "desktop" / "storage_recovery_bridge.cpp"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    open_body = source.split("void StorageRecoveryBridge::open()", 1)[1].split("void StorageRecoveryBridge::close()", 1)[0]
    required = [
        'if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); }',
        'if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); }',
        'if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }',
        'analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();',
        'QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection',
    ]
    missing = [marker for marker in required if marker not in open_body]
    if missing:
        raise AssertionError("missing storage reopen analysis reset markers: " + ", ".join(missing))
    if "C++ storage bridge now resets active analysis retryability" not in doc:
        raise AssertionError("missing storage reopen analysis reset documentation")
    print(f"storage reopen analysis reset contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
