"""Contract checks for discard confirmation races with storage disconnect."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'window.storageReopenPending || storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0',
        '"Storage became unavailable; bulk discard was canceled. Reopen storage to refresh recovery runs."',
        '"Storage became unavailable; discard of " + runId + " was canceled. Reopen storage to refresh recovery runs."',
        'reopenStorage.forceActiveFocus()',
        'storageRecovery.discardRuns(storageRecovery.retryableRunIds)',
        'storageRecovery.discardSelectedRun()',
        'objectName: "discardAllPersistedRecovery"',
        'id: discardRecoveryDialog',
    ]
    if source.count('window.storageReopenPending || storageRecovery.status === "Storage closed"') < 2:
        raise AssertionError("both discard dialogs must guard acceptance against disconnect")
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing discard dialog race markers: " + ", ".join(missing))
    if "Both selected-run and bulk discard dialogs re-check" not in doc:
        raise AssertionError("missing discard dialog race documentation")
    print(f"recovery discard dialog race contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
