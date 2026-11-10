"""Contract coverage for display recovery failure states."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_display_recovery_failures_mark_stale_and_pending() -> None:
    start = QML.index("function recoverOverlayAfterDisplayChange")
    snippet = QML[start : start + 5200]
    assert snippet.count("overlayCaptureEvidenceStale = true") >= 2
    assert "Live evidence is stale until fresh capture arrives." in snippet
    assert "overlayDisplaySelectionRequired = true" in snippet
if __name__ == "__main__":
    test_display_recovery_failures_mark_stale_and_pending()
    print("1 check passed")
