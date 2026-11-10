"""Contract coverage for native overlay restored focus workflow."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_restored_helper_activates_page_and_focuses_review() -> None:
    start = QML.index("function focusOverlayRestoredState")
    helper = QML[start : QML.index("Connections {", start)]
    assert "window.requestActivate()" in helper
    assert "overlayCaptureEvidenceStale" in helper
    assert "target.forceActiveFocus()" in helper
def test_availability_restore_uses_shared_focus_workflow() -> None:
    assert 'window.focusOverlayRestoredState("DJ overlay recovered; Show, Restore, and Reapply actions are available.", overlaySettingsRetentionReviewAction)' in QML
if __name__ == "__main__":
    test_restored_helper_activates_page_and_focuses_review()
    test_availability_restore_uses_shared_focus_workflow()
    print("2 checks passed")
