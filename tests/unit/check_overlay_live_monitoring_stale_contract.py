"""Contract coverage for live monitoring recovery status."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_live_monitoring_status_exposes_stale_boundary_during_recovery() -> None:
    marker = 'objectName: "overlayLiveMonitoringState"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 1800]
    assert "Live overlay evidence is stale until fresh capture arrives" in snippet


if __name__ == "__main__":
    test_live_monitoring_status_exposes_stale_boundary_during_recovery()
    print("1 check passed")
