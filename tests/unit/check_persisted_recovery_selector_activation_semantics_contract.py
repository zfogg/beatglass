"""Contract checks for explicit persisted recovery selector activation semantics."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'property bool persistedRecoverySelectionActivated: false',
        'function onSelectedRunIdChanged()',
        'window.persistedRecoverySelectionActivated = false',
        'window.persistedRecoverySelectionActivated = true',
        'Press Tab to reach them.',
        'Press Tab to reach retry and discard actions after selection.',
        'persistedRecoveryRuns.forceActiveFocus()',
        'if (!window.persistedRecoverySelectionActivated || storageRecovery.selectedRunId === "" || !storageRecovery.analysisRetryable)',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery activation semantics markers: " + ", ".join(missing))
    if "Selector activation is explicit" not in doc:
        raise AssertionError("missing selector activation documentation")
    print(f"persisted recovery selector activation semantics contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
