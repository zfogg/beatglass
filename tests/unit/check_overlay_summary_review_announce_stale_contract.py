"""Contract coverage for stale summary review announcement."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_summary_review_announcement_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlaySettingsSummaryReviewAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 2500]
    assert "Live evidence is stale until fresh capture arrives." in snippet
if __name__ == "__main__":
    test_summary_review_announcement_exposes_stale_boundary()
    print("1 check passed")
