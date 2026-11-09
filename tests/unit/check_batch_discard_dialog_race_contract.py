"""Contract checks for batch discard confirmation race safety."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'id: discardAnalysisBatchDialog',
        'window.analysisJobBridge.status !== "Failed" && window.analysisJobBridge.status !== "Cancelled"',
        '"Analysis batch is no longer discardable; no batch recovery was discarded."',
        'retryAnalysisBatchButton.forceActiveFocus()',
        'window.analysisJobBridge.discardBatch()',
        '"Analysis batch recovery discarded; focus remains on the recovery controls."',
        'discardAnalysisBatchButton.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing batch discard dialog race markers: " + ", ".join(missing))
    if "Batch Discard confirmation also re-checks terminal status" not in doc:
        raise AssertionError("missing batch discard dialog race documentation")
    print(f"batch discard dialog race contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
