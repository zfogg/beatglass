"""Contract coverage for active display restoration focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_active_display_restore_announces_and_focuses_retry() -> None:
    assert 'window.focusOverlayRestoredState("DJ overlay display restored; retry is available while native overlay recovery completes.", retryOverlay)' in QML
    assert "window.recoverOverlayAfterDisplayChange()" in QML
if __name__ == "__main__":
    test_active_display_restore_announces_and_focuses_retry()
    print("1 check passed")
