"""Contract coverage for stale estimate attribution naming."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_estimate_attribution_name_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayEstimateAttribution"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1000]
    assert "evidence stale until fresh capture" in snippet


if __name__ == "__main__":
    test_estimate_attribution_name_exposes_stale_boundary()
    print("1 check passed")
