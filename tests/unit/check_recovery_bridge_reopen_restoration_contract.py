"""Contract checks for reopen-storage recovery restoration and focus."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'property bool storageReopenPending: false',
        'storageRecovery.status === "Opening storage"',
        'window.storageReopenPending = true',
        'function onRetryableRunsChanged()',
        'storageRecovery.status !== "Storage open"',
        'window.storageReopenPending = false',
        '"Storage reopened; " + storageRecovery.retryableRunIds.length + " persisted recovery runs are available."',
        'persistedRecoveryRuns.forceActiveFocus()',
        '"Storage reopened; no persisted recovery runs are available."',
        'libraryTab.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing reopen restoration markers: " + ", ".join(missing))
    if "retryableRunsChanged refresh completion" not in doc:
        raise AssertionError("missing reopen restoration documentation")
    print(f"recovery bridge reopen restoration contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
