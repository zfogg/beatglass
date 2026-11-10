"""Contract coverage for centralized overlay restore failure recovery."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_restore_failures_mark_stale_and_pending() -> None:
    start = QML.index("function markOverlayRecoveryStale")
    helper = QML[start : QML.index("function restoreOverlayFallbackControls", start)]
    assert "overlayCaptureEvidenceStale = true" in helper
    assert "overlayDisplayRecoveryPending = true" in helper
    assert "Live evidence is stale until fresh capture arrives." in helper
def test_restore_paths_use_central_failure_transition() -> None:
    start = QML.index("function showOverlayAndHideControls")
    snippet = QML[start : QML.index("function hideOverlayAndRestoreControls", start)]
    assert snippet.count("window.markOverlayRecoveryStale(") >= 3
if __name__ == "__main__":
    test_restore_failures_mark_stale_and_pending()
    test_restore_paths_use_central_failure_transition()
    print("2 checks passed")
