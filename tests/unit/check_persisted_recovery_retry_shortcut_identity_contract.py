"""Contract checks for selected identity in keyboard retry shortcut feedback."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRetryShortcut"',
        'sequence: "Ctrl+Shift+Y"',
        'const runId = storageRecovery.selectedRunId',
        'storageRecovery.retrySelectedRun()',
        'window.announceVisualizationRecovery("Persisted recovery retry admitted for " + runId + " from keyboard; reset status cleared.")',
        'retryPersistedRecovery.forceActiveFocus()',
        'storageRecovery.analysisRetryable',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery retry shortcut identity markers: " + ", ".join(missing))
    print(f"persisted recovery retry shortcut identity contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
