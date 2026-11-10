"""Contract coverage for native availability focus workflow."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_availability_focus_helper_unifies_page_activation_and_focus() -> None:
    start = QML.index("function focusOverlayAvailabilityRecovery")
    helper = QML[start : QML.index("Connections {", start)]
    assert "navigation.currentIndex = 1" in helper
    assert "window.requestActivate()" in helper
    assert "target.forceActiveFocus()" in helper
    assert "Live evidence is stale until fresh capture arrives." in helper
def test_availability_loss_uses_shared_focus_workflow() -> None:
    assert 'window.focusOverlayAvailabilityRecovery("DJ overlay unavailable; output monitoring remains unavailable until the display recovers.", retryOverlay)' in QML
if __name__ == "__main__":
    test_availability_focus_helper_unifies_page_activation_and_focus()
    test_availability_loss_uses_shared_focus_workflow()
    print("2 checks passed")
