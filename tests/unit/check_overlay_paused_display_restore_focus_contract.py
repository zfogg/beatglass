"""Contract coverage for paused display restoration focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_paused_display_restore_uses_availability_focus_helper() -> None:
    assert 'window.focusOverlayAvailabilityRecovery("DJ overlay display restored; recovery remains paused until Resume recovery is chosen.", resumeOverlayRecovery)' in QML
    assert "if (!window.overlayRecoveryPausedByUser)" in QML
if __name__ == "__main__":
    test_paused_display_restore_uses_availability_focus_helper()
    print("1 check passed")
