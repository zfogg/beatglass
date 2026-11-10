"""Contract checks for live batch Cancel accessibility state."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "cancelAnalysisBatch"',
        'Accessible.description: visible && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running") ? "Cancel all queued or running analysis items retained in the current batch. Current state: " + window.analysisJobBridge.status + "; accepted items: " + window.analysisJobBridge.batchAcceptedCount + "." : "Batch cancel is unavailable because the current batch is no longer active."',
        'Current state: " + window.analysisJobBridge.status + "; accepted items: " + window.analysisJobBridge.batchAcceptedCount',
        '"Batch cancel is unavailable because the current batch is no longer active."',
        'enabled: visible && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running")',
        'window.analysisJobBridge.cancelBatch()',
        'cancelAnalysisBatch.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing batch cancel accessibility state markers: " + ", ".join(missing))
    if "Batch Cancel exposes the same live status/count value" not in doc:
        raise AssertionError("missing batch cancel accessibility documentation")
    print(f"batch cancel state accessibility contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
