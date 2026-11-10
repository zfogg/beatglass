"""Contract coverage for endpoint refresh during capture recovery."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_endpoint_refresh_exposes_stale_capture_boundary() -> None:
    marker = 'text: "Refresh overlay endpoint"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 1200]
    assert "live evidence stale" in snippet
    assert "Live overlay evidence is stale until fresh capture arrives" in snippet


if __name__ == "__main__":
    test_endpoint_refresh_exposes_stale_capture_boundary()
    print("1 check passed")
