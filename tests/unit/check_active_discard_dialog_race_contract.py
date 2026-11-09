"""Contract checks for active-worker discard confirmation race safety."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'id: discardAnalysisDialog',
        'if (!window.analysisJobBridge || !window.analysisJobBridge.canRetry)',
        '"Analysis recovery changed before discard; no recovery was discarded.',
        'retryAnalysis.forceActiveFocus()',
        'window.analysisJobBridge.discard()',
        '"Analysis recovery discarded; focus remains on the recovery controls."',
        'standardButtons: Dialog.Ok | Dialog.Cancel',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing active discard dialog race markers: " + ", ".join(missing))
    if "Active-worker Discard confirmation re-checks `canRetry`" not in doc:
        raise AssertionError("missing active discard dialog race documentation")
    print(f"active discard dialog race contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
