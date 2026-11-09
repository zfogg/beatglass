"""Contract checks for the persisted recovery discard control description."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "discardPersistedRecovery"',
        'Accessible.name: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        'Accessible.description: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"Open confirmation to remove retry record " + storageRecovery.selectedRunId + ". The source audio file is never changed."',
        '"Open confirmation to remove the selected retryable run record. The source audio file is never changed."',
        'onClicked: discardRecoveryDialog.open()',
        'id: discardRecoveryDialog',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery discard control description markers: " + ", ".join(missing))
    print(f"persisted recovery discard control description contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
