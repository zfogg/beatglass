"""Contract checks for active-job rejection versus persisted recovery outcomes."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'property string activeAnalysisOutcomeAnnouncementKey: ""',
        'if (storageRecovery.selectedRunId === "")',
        'const activeOutcomeKey = status',
        'window.activeAnalysisOutcomeAnnouncementKey = activeOutcomeKey',
        '"Active analysis failed; Retry analysis is available."',
        '"Active analysis was canceled; Retry analysis is available."',
        'retryAdmittedAnalysis.forceActiveFocus()',
        'window.activeAnalysisOutcomeAnnouncementKey = ""',
        'window.persistedRecoveryOutcomeAnnouncementKey = ""',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing active-job outcome markers: " + ", ".join(missing))
    if "Active-job rejection is isolated from persisted-run recovery" not in doc:
        raise AssertionError("missing active-job outcome documentation")
    print(f"persisted recovery active-job outcomes contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
