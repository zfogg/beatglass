"""Contract checks for count-aware persisted recovery selector semantics."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRuns"',
        'Accessible.name: "Persisted retryable analysis runs (" + count + ")"',
        'Accessible.role: Accessible.ComboBox',
        'enabled: visible && count > 0',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
        'storageRecovery.selectRun(currentText)',
        'persistedRecoveryRuns.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery selector count markers: " + ", ".join(missing))
    print(f"persisted recovery selector count contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
