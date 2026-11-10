"""Contract coverage for the manual recovery pause transition."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_pause_transition_preserves_monitoring_and_focuses_resume() -> None:
    start = QML.index("function stopOverlayRecoveryAttempts")
    snippet = QML[start : QML.index("function quitApplication", start)]
    assert "overlayRecoveryPausedByUser = true" in snippet
    assert "overlayCaptureEvidenceStale = true" in snippet
    assert "window.requestActivate()" in snippet
    assert "resumeOverlayRecovery.forceActiveFocus()" in snippet
    assert "Live evidence is stale until fresh capture arrives." in snippet


if __name__ == "__main__":
    test_pause_transition_preserves_monitoring_and_focuses_resume()
    print("1 check passed")
