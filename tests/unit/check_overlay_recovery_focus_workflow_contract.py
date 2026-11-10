"""Contract coverage for cohesive recovery focus workflow."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_recovery_focus_helper_unifies_navigation_focus_and_boundary() -> None:
    assert "function focusOverlayRecoveryAction(message)" in QML
    helper = QML[QML.index("function focusOverlayRecoveryAction") : QML.index("function restoreOverlayDisplaySelectionFromBridge")]
    assert "navigation.currentIndex = 1" in helper
    assert "window.requestActivate()" in helper
    assert "resumeOverlayRecovery.forceActiveFocus()" in helper
    assert "window.overlayRecoveryEvidenceLabel" in helper
if __name__ == "__main__":
    test_recovery_focus_helper_unifies_navigation_focus_and_boundary()
    print("1 check passed")
