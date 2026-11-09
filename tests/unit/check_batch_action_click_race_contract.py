"""Contract checks for batch retry/cancel click-time status guards."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "retryAnalysisBatch"',
        'window.analysisJobBridge.status !== "Failed" && window.analysisJobBridge.status !== "Cancelled"',
        '"Analysis batch is no longer retryable; no batch retry was dispatched."',
        'window.analysisJobBridge.retryBatch()',
        'retryAnalysisBatchButton.forceActiveFocus()',
        'objectName: "cancelAnalysisBatch"',
        'window.analysisJobBridge.status !== "Queued" && window.analysisJobBridge.status !== "Running"',
        '"Analysis batch is no longer active; no batch cancel was dispatched."',
        'window.analysisJobBridge.cancelBatch()',
        'cancelAnalysisBatch.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing batch action click race markers: " + ", ".join(missing))
    if "Batch Retry and Cancel use the same click-time status guards" not in doc:
        raise AssertionError("missing batch click race documentation")
    print(f"batch action click race contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
