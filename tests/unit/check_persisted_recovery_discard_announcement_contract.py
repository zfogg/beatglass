"""Contract checks for selected identity in discard announcements."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "discardRecoveryDialog"',
        'const runId = storageRecovery.selectedRunId',
        'storageRecovery.discardSelectedRun()',
        'window.announceVisualizationRecovery("Persisted recovery run " + runId + " discarded; selected retry state cleared.")',
        'discardPersistedRecoveryButton.forceActiveFocus()',
        'onRejected: { window.announceVisualizationRecovery("Persisted recovery discard canceled; selected run was kept.")',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery discard announcement markers: " + ", ".join(missing))
    print(f"persisted recovery discard announcement contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
