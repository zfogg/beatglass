"""Contract checks for clearing stale admission state before storage reopen."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
SOURCE = ROOT / "src" / "desktop" / "storage_recovery_bridge.cpp"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    open_body = source.split("void StorageRecoveryBridge::open()", 1)[1].split("void StorageRecoveryBridge::close()", 1)[0]
    required = [
        'if (analysisReserved_) { analysisReserved_ = false; emit analysisAdmissionChanged(); }',
        'if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }',
        'QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection',
        'analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();',
    ]
    missing = [marker for marker in required if marker not in open_body]
    if missing:
        raise AssertionError("missing storage reopen admission reset markers: " + ", ".join(missing))
    if "analysisReserved" not in doc or "analysisAdmissionResult" not in doc:
        raise AssertionError("missing reopen admission reset documentation")
    print(f"storage reopen admission reset contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
