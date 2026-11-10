"""Contract coverage for display-selection recovery focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_selection_required_uses_availability_focus_workflow() -> None:
    assert 'window.focusOverlayAvailabilityRecovery("DJ overlay recovered; choose a display to re-enable overlay actions.", overlayDisplay)' in QML
if __name__ == "__main__":
    test_selection_required_uses_availability_focus_workflow()
    print("1 check passed")
