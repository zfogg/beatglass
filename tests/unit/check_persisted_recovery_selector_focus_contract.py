"""Contract checks for keyboard focus semantics on persisted recovery selector."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRuns"',
        'activeFocusOnTab: true',
        'Accessible.role: Accessible.ComboBox',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
        'storageRecovery.selectRun(currentText)',
        'persistedRecoveryRuns.forceActiveFocus()',
        'window.announceVisualizationRecovery("Selected persisted recovery run " + currentText',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery selector focus markers: " + ", ".join(missing))
    print(f"persisted recovery selector focus contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
