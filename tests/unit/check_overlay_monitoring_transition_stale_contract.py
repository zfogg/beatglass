"""Contract coverage for monitoring transition accessibility."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_monitoring_action_transition_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayMonitoringAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 3200]
    assert "live overlay evidence is stale until the current capture state produces a fresh accepted snapshot" in snippet


if __name__ == "__main__":
    test_monitoring_action_transition_exposes_stale_boundary()
    print("1 check passed")
