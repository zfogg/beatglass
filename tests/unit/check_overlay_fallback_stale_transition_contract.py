"""Contract coverage for fallback-control stale transition."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_fallback_transition_marks_stale_and_announces_boundary() -> None:
    start = QML.index("function restoreOverlayFallbackControls")
    snippet = QML[start : QML.index("function showOverlayAndHideControls", start)]
    assert "overlayLaunchFallbackActive = true" in snippet
    assert "overlayDisplayRecoveryPending = true" in snippet
    assert "overlayCaptureEvidenceStale = true" in snippet
    assert "Live evidence is stale until fresh capture arrives." in snippet
if __name__ == "__main__":
    test_fallback_transition_marks_stale_and_announces_boundary()
    print("1 check passed")
