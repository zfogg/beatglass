"""Contract checks for reset status context on persisted recovery selector."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRuns"',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
        'storageRecovery.resetStatus !== undefined && storageRecovery.resetStatus !== ""',
        '"Recovery status: " + storageRecovery.resetStatus + ". "',
        '"Retry and discard controls are available when the run is retryable."',
        '"Choose a saved analysis run to recover."',
        'storageRecovery.selectRun(currentText)',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery selector status markers: " + ", ".join(missing))
    print(f"persisted recovery selector status contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
