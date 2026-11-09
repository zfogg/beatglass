"""Contract checks for complete state reset on explicit storage close."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
SOURCE = ROOT / "src" / "desktop" / "storage_recovery_bridge.cpp"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    close_body = source.split("void StorageRecoveryBridge::close()", 1)[1].split("void StorageRecoveryBridge::refreshRetryableRuns", 1)[0]
    required = [
        'if (cacheHit_) { cacheHit_ = false; emit cachedResultChanged(); }',
        'if (!cachedResult_.isEmpty()) { cachedResult_.clear(); emit cachedResultChanged(); }',
        'if (analysisReserved_) { analysisReserved_ = false; emit analysisAdmissionChanged(); }',
        'if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }',
        'if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }',
        'if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); }',
        'status_ = QStringLiteral("Storage closed"); emit statusChanged();',
    ]
    missing = [marker for marker in required if marker not in close_body]
    if missing:
        raise AssertionError("missing storage close result reset markers: " + ", ".join(missing))
    if "Explicit `StorageRecoveryBridge::close()` applies the same" not in doc:
        raise AssertionError("missing storage close result reset documentation")
    print(f"storage close result reset contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
