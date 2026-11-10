"""Contract coverage for retry monitoring stale-evidence affordance."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_retry_monitor_affordance_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayRetryCapture"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 5200]
    assert "previous evidence is stale until fresh capture" in snippet
    assert "previous evidence is stale until fresh capture arrives." in snippet


if __name__ == "__main__":
    test_retry_monitor_affordance_exposes_stale_boundary()
    print("1 check passed")
