"""Contract coverage for empty-display stale state."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_empty_display_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayDisplayEmptyState"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 700]
    assert "live evidence stale until fresh capture" in snippet


if __name__ == "__main__":
    test_empty_display_exposes_stale_boundary()
    print("1 check passed")
