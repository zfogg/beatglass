"""Contract checks for active-worker cancel click-time safety."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "cancelAnalysis"',
        'window.analysisJobBridge.canCancel',
        '"Analysis is no longer active; cancel was not dispatched."',
        'retryAnalysis.forceActiveFocus()',
        'window.analysisJobBridge.cancel()',
        '"Analysis cancelled; waiting for a replacement accepted snapshot."',
        'cancelAnalysis.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing active cancel click race markers: " + ", ".join(missing))
    if "active Cancel control also re-checks" not in doc:
        raise AssertionError("missing active cancel click race documentation")
    print(f"active cancel click race contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
