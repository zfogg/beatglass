"""Contract coverage for deterministic safe-mode reset transition."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_safe_mode_reset_restores_page_focus_and_boundary() -> None:
    start = QML.index("function resetOverlayRecoveryAttempts")
    snippet = QML[start : QML.index("function stopOverlayRecoveryAttempts", start)]
    assert "overlayRecoveryAttemptCount = 0" in snippet
    assert "window.show()" in snippet
    assert "window.requestActivate()" in snippet
    assert "retryOverlay.forceActiveFocus()" in snippet
    assert "Live evidence remains stale until fresh capture arrives." in snippet
if __name__ == "__main__":
    test_safe_mode_reset_restores_page_focus_and_boundary()
    print("1 check passed")
