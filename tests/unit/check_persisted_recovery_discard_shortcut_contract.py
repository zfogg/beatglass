"""Contract checks for the safe keyboard entry point to persisted discard."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryDiscardShortcut"',
        'sequence: "Ctrl+Shift+D"',
        'storageRecovery.selectedRunId !== ""',
        'discardPersistedRecoveryButton.forceActiveFocus()',
        'window.announceVisualizationRecovery("Persisted recovery discard focused; press Enter to review the confirmation dialog.")',
        'id: discardPersistedRecoveryButton',
        'onClicked: discardRecoveryDialog.open()',
        'onAccepted: { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery discard shortcut markers: " + ", ".join(missing))
    print(f"persisted recovery discard shortcut contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
