"""Contract coverage for settings review stale affordance."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_settings_review_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlaySettingsRetentionReviewAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1300]
    assert "evidence stale until fresh capture" in snippet
    assert "Previously displayed evidence is stale until fresh capture arrives." in snippet


if __name__ == "__main__":
    test_settings_review_exposes_stale_boundary()
    print("1 check passed")
