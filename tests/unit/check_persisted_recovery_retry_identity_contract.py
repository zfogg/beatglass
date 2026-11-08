"""Contract checks for selected identity on persisted recovery retry."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "retryPersistedRecovery"',
        'Accessible.name: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"Retry selected persisted analysis run " + storageRecovery.selectedRunId',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
        'storageRecovery.analysisRetryable',
        'storageRecovery.retrySelectedRun()',
        'window.announceVisualizationRecovery("Persisted recovery retry admitted; reset status cleared.")',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery retry identity markers: " + ", ".join(missing))
    print(f"persisted recovery retry identity contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
