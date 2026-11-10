"""Contract coverage for monitoring action stale description."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_monitor_action_description_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayMonitoringAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 3600]
    assert "Live overlay evidence is stale until fresh capture arrives" in snippet


if __name__ == "__main__":
    test_monitor_action_description_exposes_stale_boundary()
    print("1 check passed")
