"""Contract checks for selected identity in keyboard discard shortcut feedback."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryDiscardShortcut"',
        'sequence: "Ctrl+Shift+D"',
        'const runId = storageRecovery.selectedRunId',
        'discardPersistedRecoveryButton.forceActiveFocus()',
        'window.announceVisualizationRecovery("Persisted recovery discard focused for " + runId + "; press Enter to review the confirmation dialog.")',
        'storageRecovery.selectedRunId !== ""',
        'context: Qt.ApplicationShortcut',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery discard shortcut identity markers: " + ", ".join(missing))
    print(f"persisted recovery discard shortcut identity contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
