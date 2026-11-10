"""Contract coverage for stale forget-display recovery affordance."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_forget_display_exposes_stale_boundary() -> None:
    marker = 'objectName: "forgetUnavailableOverlayDisplay"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 900]
    assert "Live overlay evidence is stale until fresh capture arrives." in snippet
    assert "live evidence is stale until fresh capture" in snippet


if __name__ == "__main__":
    test_forget_display_exposes_stale_boundary()
    print("1 check passed")
