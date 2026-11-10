"""Contract checks for live batch action accessibility state."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "retryAnalysisBatch"',
        'Accessible.description: visible ? text + ". " + window.analysisJobBridge.batchAcceptedCount + " accepted item(s) are represented; actions retain source audio and return focus after completion." : "No analysis batch actions are available."',
        'Current state: " + window.analysisJobBridge.status',
        'objectName: "discardAnalysisBatch"',
        'Accessible.description: visible ? "Clear the terminal batch recovery state without changing source audio.',
        'window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled"',
        'window.analysisJobBridge.batchAcceptedCount > 0',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing batch action accessibility state markers: " + ", ".join(missing))
    if "Batch Retry and Discard controls expose the live worker status" not in doc:
        raise AssertionError("missing batch action accessibility documentation")
    print(f"batch action state accessibility contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
