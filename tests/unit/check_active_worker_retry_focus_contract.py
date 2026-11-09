"""Contract checks for actual-worker retry gating and terminal focus restoration."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'window.analysisJobBridge.retry !== undefined && window.analysisJobBridge.canRetry',
        'storageRecovery.retryAnalysis !== undefined && storageRecovery.analysisRetryable',
        'window.analysisJobBridge.status === "Failed"',
        'window.analysisJobBridge.status === "Cancelled"',
        '"Analysis " + window.analysisJobBridge.status.toLowerCase() + "; Retry analysis is available."',
        'retryAnalysis.forceActiveFocus()',
        'enabled: visible && window.analysisJobBridge.canRetry',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing active worker retry markers: " + ", ".join(missing))
    if "actual worker handle" not in doc or "canRetry" not in doc:
        raise AssertionError("missing active worker retry documentation")
    print(f"active worker retry focus contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
