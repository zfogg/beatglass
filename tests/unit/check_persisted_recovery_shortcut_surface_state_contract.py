"""Contract checks for persisted recovery shortcut surface-state guards."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRetryShortcut"',
        'objectName: "persistedRecoveryDiscardShortcut"',
        'persistedRecoveryRuns.enabled',
        'storageRecovery.selectedRunId !== ""',
        'storageRecovery.analysisRetryable',
        'retryPersistedRecovery.forceActiveFocus()',
        'discardPersistedRecoveryButton.forceActiveFocus()',
    ]
    if source.count('&& persistedRecoveryRuns.enabled') < 2:
        raise AssertionError("retry and discard shortcuts must both require an enabled recovery surface")
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery shortcut surface-state markers: " + ", ".join(missing))
    print(f"persisted recovery shortcut surface-state contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
