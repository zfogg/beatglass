"""Contract checks for persisted retry click-time disconnect safety."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "retryPersistedRecovery"',
        'const runId = storageRecovery.selectedRunId',
        'window.storageReopenPending || storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0',
        '"Storage became unavailable; retry of " + runId + " was canceled. Reopen storage to refresh recovery runs."',
        'reopenStorage.forceActiveFocus()',
        'storageRecovery.retrySelectedRun()',
        'window.announceVisualizationRecovery("Persisted recovery retry admitted for " + runId',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted retry click race markers: " + ", ".join(missing))
    if "Persisted Retry applies the same click-time check" not in doc:
        raise AssertionError("missing persisted retry click race documentation")
    print(f"recovery retry click race contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
