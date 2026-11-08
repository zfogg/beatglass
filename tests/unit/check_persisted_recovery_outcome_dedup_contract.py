"""Contract checks for real-status persisted recovery announcement de-duplication."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'property string persistedRecoveryOutcomeAnnouncementKey: ""',
        'function onAnalysisStatusChanged()',
        'const status = storageRecovery.analysisStatus',
        'const outcomeKey = storageRecovery.selectedRunId + "|" + status',
        'if (window.persistedRecoveryOutcomeAnnouncementKey === outcomeKey)',
        'window.persistedRecoveryOutcomeAnnouncementKey = ""',
        'storageRecovery.analysisStatus === "Analysis failed"',
        'storageRecovery.analysisStatus === "Analysis cancelled"',
        'retryPersistedRecovery.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery outcome de-dup markers: " + ", ".join(missing))
    if "analysisStatus" not in doc or "selected-run/status pair once" not in doc:
        raise AssertionError("missing persisted recovery outcome documentation")
    print(f"persisted recovery outcome de-dup contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
