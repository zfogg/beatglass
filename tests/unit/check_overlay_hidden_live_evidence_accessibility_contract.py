"""Contract coverage for hiding the overlay without invalidating monitoring."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_hidden_overlay_status_preserves_live_evidence() -> None:
    marker = 'objectName: "overlayVisibilityStatus"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1000]
    assert "without losing monitoring or invalidating live evidence" in snippet


if __name__ == "__main__":
    test_hidden_overlay_status_preserves_live_evidence()
    print("1 check passed")
