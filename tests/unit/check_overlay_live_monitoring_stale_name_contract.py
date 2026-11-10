"""Contract coverage for live monitoring stale-state naming."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_live_monitoring_name_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayLiveMonitoringState"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1100]
    assert "evidence stale until fresh capture" in snippet


if __name__ == "__main__":
    test_live_monitoring_name_exposes_stale_boundary()
    print("1 check passed")
