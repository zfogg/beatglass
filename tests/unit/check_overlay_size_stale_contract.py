"""Contract coverage for stale overlay size selector."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_size_selector_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlaySize"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 650]
    assert "live evidence stale until fresh capture" in snippet
    assert "Live evidence is stale until fresh capture arrives." in snippet
if __name__ == "__main__":
    test_size_selector_exposes_stale_boundary()
    print("1 check passed")
