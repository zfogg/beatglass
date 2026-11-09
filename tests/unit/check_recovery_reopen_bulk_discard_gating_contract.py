"""Contract checks for bulk discard gating during storage reopen."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "discardAllPersistedRecovery"',
        'enabled: visible && !window.storageReopenPending && storageRecovery.retryableRunIds.length > 0',
        'storageRecovery.discardRuns !== undefined',
        'storageRecovery.retryableRunIds.length > 0',
        'objectName: "discardAllPersistedRecovery"',
        'onAccepted: { storageRecovery.discardRuns(storageRecovery.retryableRunIds)',
        'window.storageReopenPending = true',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing bulk discard reopen gating markers: " + ", ".join(missing))
    if "same pending gate covers bulk Discard all" not in doc:
        raise AssertionError("missing bulk discard gating documentation")
    print(f"recovery reopen bulk discard gating contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
