"""Contract checks for active worker de-duplication lifecycle reset."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'target: window.analysisJobBridge',
        'function onCanRetryChanged()',
        'if (!window.analysisJobBridge.canRetry)',
        'window.activeWorkerOutcomeAnnouncementKey = ""',
        'function onStatusChanged()',
        'status !== "Failed" && status !== "Cancelled"',
        'window.analysisJobBridge.canRetry',
        'retryAnalysis.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing active worker lifecycle reset markers: " + ", ".join(missing))
    if "canRetry: false" not in doc or "close/reopen" not in doc:
        raise AssertionError("missing active worker lifecycle reset documentation")
    print(f"active worker lifecycle reset contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
