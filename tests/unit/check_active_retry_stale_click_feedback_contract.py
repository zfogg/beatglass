"""Contract checks for stale active Retry click feedback."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'function retryVisualizationAnalysis()',
        'window.analysisJobBridge.retry !== undefined && window.analysisJobBridge.canRetry',
        'storageRecovery.retryAnalysis !== undefined && storageRecovery.analysisRetryable',
        '"Analysis retry is no longer available; the worker state changed before admission."',
        'retryAnalysis.forceActiveFocus()',
        'return false',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing active retry stale-click markers: " + ", ".join(missing))
    if "Active Retry now reports the analogous stale-click outcome" not in doc:
        raise AssertionError("missing active retry stale-click documentation")
    print(f"active retry stale-click feedback contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
