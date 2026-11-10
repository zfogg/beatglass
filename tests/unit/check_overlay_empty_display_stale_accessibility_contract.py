"""Contract coverage for no-display overlay recovery messaging."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_empty_display_state_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayDisplayEmptyState"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 600]
    assert "Live overlay evidence is stale while no display is available" in snippet
    assert "monitoring continues in the control window" in snippet


if __name__ == "__main__":
    test_empty_display_state_exposes_stale_boundary()
    print("1 check passed")
