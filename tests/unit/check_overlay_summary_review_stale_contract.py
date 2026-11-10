"""Contract coverage for stale overlay summary review action."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_summary_review_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlaySettingsSummaryReviewAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1300]
    assert "live evidence stale until fresh capture" in snippet
    assert "live overlay evidence is stale until fresh capture" in snippet
if __name__ == "__main__":
    test_summary_review_exposes_stale_boundary()
    print("1 check passed")
