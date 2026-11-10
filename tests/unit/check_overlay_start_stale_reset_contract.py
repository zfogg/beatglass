"""Contract coverage for starting overlay monitoring."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_monitoring_start_marks_prior_evidence_stale() -> None:
    marker = 'objectName: "overlayMonitoringAction"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 3500]
    assert "overlayCaptureEvidenceStale = true" in snippet
    assert "Overlay monitoring requested; waiting for accepted output data." in snippet


if __name__ == "__main__":
    test_monitoring_start_marks_prior_evidence_stale()
    print("1 check passed")
