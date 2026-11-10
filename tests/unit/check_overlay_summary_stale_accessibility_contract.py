"""Contract coverage for overlay summary recovery semantics."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_overlay_summary_exposes_stale_recovery_boundary() -> None:
    marker = 'objectName: "overlaySettingsSummaryReviewAction"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 1600]
    assert "recovery pending; live overlay evidence stale" in snippet
    assert "live overlay evidence is stale" in snippet


if __name__ == "__main__":
    test_overlay_summary_exposes_stale_recovery_boundary()
    print("1 check passed")
