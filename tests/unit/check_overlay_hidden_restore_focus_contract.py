"""Contract coverage for hidden-by-user display restoration focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_hidden_restore_focuses_restore_action() -> None:
    assert 'window.focusOverlayRestoredState("DJ overlay display recovered" + hiddenGeometrySummary + "; it remains hidden by user request. Use Restore overlay when ready.", restoreOverlay)' in QML
if __name__ == "__main__":
    test_hidden_restore_focuses_restore_action()
    print("1 check passed")
