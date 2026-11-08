"""Contract checks for the persisted recovery retry control description."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "retryPersistedRecovery"',
        'Accessible.name: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        'Accessible.description: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"Retry persisted run " + storageRecovery.selectedRunId + " when recovery state confirms retryable work.',
        '"Retry the selected persisted run only when recovery state confirms retryable work.',
        'storageRecovery.analysisRetryable',
        'storageRecovery.retrySelectedRun()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery retry control description markers: " + ", ".join(missing))
    print(f"persisted recovery retry control description contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
