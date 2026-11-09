"""Contract checks for selected identity exposure in persisted discard dialog."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'title: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        'Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""',
        'text: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"This removes only retry record " + storageRecovery.selectedRunId + ". The source audio file is not changed."',
        'Accessible.name: "Discard retryable run confirmation"',
        'standardButton(Dialog.Cancel).forceActiveFocus()',
        'onAccepted: { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery dialog identity markers: " + ", ".join(missing))
    print(f"persisted recovery dialog identity contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
