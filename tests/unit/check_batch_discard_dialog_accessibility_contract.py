"""Contract checks for batch discard dialog accessibility and safe focus."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "discardAnalysisBatchDialog"',
        'Accessible.role: Accessible.StaticText',
        'Accessible.name: "Discard analysis batch confirmation"',
        'Accessible.description: "Clear the retained batch recovery state without changing source audio. Cancel is focused first; Tab reaches the destructive OK action."',
        'onOpened: { window.announceVisualizationRecovery("Confirm discard of the analysis batch; source audio will remain unchanged."); standardButton(Dialog.Cancel).forceActiveFocus() }',
        'standardButtons: Dialog.Ok | Dialog.Cancel',
        'onRejected: discardAnalysisBatchButton.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing batch discard accessibility markers: " + ", ".join(missing))
    if "The batch discard dialog exposes an explicit accessible Dialog role" not in doc:
        raise AssertionError("missing batch discard accessibility documentation")
    print(f"batch discard dialog accessibility contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
