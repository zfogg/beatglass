"""Contract coverage for retaining stale evidence through capture retry."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_capture_retry_marks_evidence_stale_before_start() -> None:
    marker = 'objectName: "overlayCaptureRecoveryAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1800]
    assert "overlayCaptureEvidenceStale = true" in snippet
    assert "captureMonitorBridge.start()" in snippet


if __name__ == "__main__":
    test_capture_retry_marks_evidence_stale_before_start()
    print("1 check passed")
