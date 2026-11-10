"""Contract coverage for stale summary review value."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_summary_review_value_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlaySettingsSummaryReviewAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1300]
    assert "live overlay evidence stale until fresh capture" in snippet
if __name__ == "__main__":
    test_summary_review_value_exposes_stale_boundary()
    print("1 check passed")
