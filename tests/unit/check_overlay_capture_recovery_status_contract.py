"""Contract coverage for the overlay capture recovery status card."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_capture_recovery_status_exposes_no_estimates_boundary() -> None:
    marker = 'objectName: "overlayCaptureRecoveryStatus"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 1000]
    assert "Monitoring paused · reacquiring selected output" in snippet
    assert "selected output unavailable" in snippet
    assert "no estimates" in snippet
    assert "No new estimates are published while the output is unavailable" in snippet


if __name__ == "__main__":
    test_capture_recovery_status_exposes_no_estimates_boundary()
    print("1 check passed")
