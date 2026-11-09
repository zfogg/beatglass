"""Contract checks for stale terminal signals across persisted run changes."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'property bool persistedRecoverySelectionActivated: false',
        'function onSelectedRunIdChanged()',
        'window.persistedRecoverySelectionActivated = false',
        'window.persistedRecoveryOutcomeAnnouncementKey = ""',
        'if (!window.persistedRecoverySelectionActivated || storageRecovery.selectedRunId === "" || !storageRecovery.analysisRetryable)',
        'window.persistedRecoverySelectionActivated = true',
        'storageRecovery.analysisStatus === "Analysis failed"',
        'storageRecovery.analysisStatus === "Analysis cancelled"',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery signal guard markers: " + ", ".join(missing))
    if "Rapid persisted-run changes are guarded separately" not in doc:
        raise AssertionError("missing rapid-selection guard documentation")
    print(f"persisted recovery selection signal guard contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
