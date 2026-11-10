"""Contract coverage for unavailable overlay stale evidence."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_unavailable_overlay_status_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayUnavailable"'
    snippet = QML[QML.index(marker) - 200 : QML.index(marker) + 600]
    assert "live overlay evidence stale" in snippet
    assert "Live overlay evidence is stale" in snippet
    assert "monitoring continues in the control window" in snippet


if __name__ == "__main__":
    test_unavailable_overlay_status_exposes_stale_boundary()
    print("1 check passed")
