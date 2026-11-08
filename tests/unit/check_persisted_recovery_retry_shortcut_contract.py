"""Contract checks for the keyboard retry path for persisted recovery."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRetryShortcut"',
        'sequence: "Ctrl+Shift+Y"',
        'storageRecovery.selectedRunId !== ""',
        'storageRecovery.analysisRetryable',
        'storageRecovery.retrySelectedRun()',
        'window.announceVisualizationRecovery("Persisted recovery retry admitted from keyboard; reset status cleared.")',
        'retryPersistedRecovery.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery retry shortcut markers: " + ", ".join(missing))
    print(f"persisted recovery retry shortcut contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
