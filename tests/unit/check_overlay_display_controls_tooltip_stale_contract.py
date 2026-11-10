"""Contract coverage for stale display controls tooltip."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_display_controls_tooltip_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayDisplayControls"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1400]
    assert "live evidence is stale until fresh capture" in snippet
if __name__ == "__main__":
    test_display_controls_tooltip_exposes_stale_boundary()
    print("1 check passed")
