"""Contract checks for selected identity in retry announcements."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "retryPersistedRecovery"',
        'const runId = storageRecovery.selectedRunId',
        'storageRecovery.retrySelectedRun()',
        'window.announceVisualizationRecovery("Persisted recovery retry admitted for " + runId + "; reset status cleared.")',
        'forceActiveFocus()',
        'storageRecovery.analysisRetryable',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery retry announcement markers: " + ", ".join(missing))
    print(f"persisted recovery retry announcement contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
