"""Contract checks for the accessible persisted recovery confirmation dialog."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "discardRecoveryDialog"',
        'Accessible.role: Accessible.Dialog',
        'Accessible.name: "Discard saved analysis run confirmation"',
        'text: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"This removes only retry record " + storageRecovery.selectedRunId + ". The source audio file is not changed."',
        'Accessible.role: Accessible.StaticText',
        'onOpened: window.announceVisualizationRecovery("Confirm discard for persisted recovery run " + storageRecovery.selectedRunId',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept.")',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery dialog role markers: " + ", ".join(missing))
    print(f"persisted recovery dialog role contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
