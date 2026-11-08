"""Contract checks for active worker terminal outcome de-duplication."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'property string activeWorkerOutcomeAnnouncementKey: ""',
        'window.activeWorkerOutcomeAnnouncementKey = ""',
        'const status = window.analysisJobBridge.status',
        'if (status !== "Failed" && status !== "Cancelled")',
        'window.activeWorkerOutcomeAnnouncementKey === status',
        'window.activeWorkerOutcomeAnnouncementKey = status',
        'window.analysisJobBridge.canRetry',
        'retryAnalysis.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing active worker outcome de-dup markers: " + ", ".join(missing))
    if "de-duplicated by status" not in doc or "new non-terminal state" not in doc:
        raise AssertionError("missing active worker de-duplication documentation")
    print(f"active worker outcome de-dup contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
