"""Contract checks for aggregate live batch status accessibility."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "analysisBatchStatus"',
        'Accessible.name: "Batch analysis queue status: " + text',
        'Accessible.description: visible ? "Live batch lifecycle and result counts: " + text',
        'Accessible.role: Accessible.StaticText',
        'window.analysisJobBridge.batchAcceptedCount',
        'window.analysisJobBridge.batchFailedCount',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing live batch status accessibility markers: " + ", ".join(missing))
    if "aggregate Batch analysis queue status now exposes" not in doc:
        raise AssertionError("missing live batch status documentation")
    print(f"batch status live accessibility contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
