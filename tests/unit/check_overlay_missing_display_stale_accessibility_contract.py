"""Contract coverage for a missing saved overlay display."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_missing_saved_display_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayMissingDisplayRecovery"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1000]
    assert "live overlay evidence stale" in snippet
    assert "Live overlay evidence is stale while the saved display is missing" in snippet
    assert "monitoring continues in the control window" in snippet


if __name__ == "__main__":
    test_missing_saved_display_exposes_stale_boundary()
    print("1 check passed")
