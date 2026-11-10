"""Contract coverage for stale click-through description."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_clickthrough_description_exposes_stale_boundary() -> None:
    marker = 'objectName: "clickThrough"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1100]
    assert "Live evidence is stale until fresh capture arrives." in snippet
if __name__ == "__main__":
    test_clickthrough_description_exposes_stale_boundary()
    print("1 check passed")
