"""Contract checks for storage bridge disconnect/reopen recovery reset."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'function onStatusChanged()',
        'storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0',
        'window.activeWorkerOutcomeAnnouncementKey = ""',
        'window.persistedRecoveryOutcomeAnnouncementKey = ""',
        'window.persistedRecoverySelectionActivated = false',
        'reopenStorage.forceActiveFocus()',
        '"Storage is unavailable; reopen storage before retrying analysis recovery."',
    ]
    if source.count('function onStatusChanged()') < 3:
        raise AssertionError("expected storage, active-worker, and existing status connections")
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing bridge disconnect reset markers: " + ", ".join(missing))
    if "Storage bridge disconnects are an explicit recovery boundary" not in doc:
        raise AssertionError("missing bridge disconnect documentation")
    print(f"recovery bridge disconnect reset contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
