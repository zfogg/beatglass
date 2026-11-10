"""Contract coverage for display-loss recovery focus transition."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_display_loss_marks_stale_and_uses_recovery_focus() -> None:
    start = QML.index("function onScreensChanged")
    snippet = QML[start : QML.index("else {", start)]
    assert "overlayDisplayRecoveryPending = true" in snippet
    assert "overlayDisplaySelectionRequired = true" in snippet
    assert "overlayCaptureEvidenceStale = true" in snippet
    assert "focusOverlayAvailabilityRecovery" in snippet
if __name__ == "__main__":
    test_display_loss_marks_stale_and_uses_recovery_focus()
    print("1 check passed")
