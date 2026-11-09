"""Contract checks for persisted recovery selector pointer guidance."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRuns"',
        'ToolTip.text: count > 0',
        '"Choose one of " + count + " saved analysis runs"',
        '". Selected: " + storageRecovery.selectedRunId',
        '"No saved analysis runs available"',
        'Accessible.name: "Persisted retryable analysis runs (" + count + ")"',
        'storageRecovery.selectRun(currentText)',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery selector tooltip markers: " + ", ".join(missing))
    print(f"persisted recovery selector tooltip contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
