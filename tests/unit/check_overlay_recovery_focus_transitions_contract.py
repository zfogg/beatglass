"""Contract coverage for keyboard recovery focus transitions."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_focus_helper_handles_recovery_transition() -> None:
    start = QML.index("function focusOverlayRecoveryAction")
    helper = QML[start : QML.index("Shortcut {", start)]
    assert "navigation.currentIndex = 1" in helper
    assert "window.requestActivate()" in helper
    assert "resumeOverlayRecovery.forceActiveFocus()" in helper
    assert "Live evidence is stale until fresh capture arrives." in helper
def test_keyboard_paths_use_shared_transition() -> None:
    assert 'window.focusOverlayRecoveryAction("Paused overlay recovery; Resume recovery focused.")' in QML
    assert 'window.focusOverlayRecoveryAction("Paused overlay recovery page opened; Resume recovery is ready.")' in QML
if __name__ == "__main__":
    test_focus_helper_handles_recovery_transition()
    test_keyboard_paths_use_shared_transition()
    print("2 checks passed")
