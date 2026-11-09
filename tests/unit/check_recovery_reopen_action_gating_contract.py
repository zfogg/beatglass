"""Contract checks for action gating while storage reopen refresh is pending."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'property bool storageReopenPending: false',
        'enabled: visible && count > 0 && !window.storageReopenPending',
        'enabled: visible && !window.storageReopenPending && storageRecovery.selectedRunId !== ""',
        'enabled: visible && !window.storageReopenPending && storageRecovery.selectedRunId !== "" && storageRecovery.status',
        'window.storageReopenPending = true',
        'window.storageReopenPending = false',
        'function onRetryableRunsChanged()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing reopen action gating markers: " + ", ".join(missing))
    if "While `storageReopenPending` is true" not in doc:
        raise AssertionError("missing reopen action gating documentation")
    print(f"recovery reopen action gating contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
