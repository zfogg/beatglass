"""Contract coverage for stale display move failure."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_display_move_failure_uses_stale_transition() -> None:
    start = QML.index("if (!window.overlayBridge.moveToScreen(index))")
    snippet = QML[start : start + 700]
    assert "markOverlayRecoveryStale" in snippet
    assert "overlayDisplaySelectionRequired = true" in snippet
    assert "overlayDisplay.forceActiveFocus()" in snippet
if __name__ == "__main__":
    test_display_move_failure_uses_stale_transition()
    print("1 check passed")
