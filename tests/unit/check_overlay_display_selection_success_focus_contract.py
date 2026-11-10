"""Contract coverage for successful display selection recovery focus."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_selection_success_focuses_settings_review() -> None:
    assert 'window.focusOverlayRestoredState("DJ overlay display selection persisted for " + name + "; Show, Restore, and Reapply actions are enabled.", overlaySettingsRetentionReviewAction)' in QML
if __name__ == "__main__":
    test_selection_success_focuses_settings_review()
    print("1 check passed")
