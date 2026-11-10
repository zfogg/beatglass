"""Contract coverage for centralized capture evidence transitions."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_capture_transition_helper_controls_state_announcement_and_focus() -> None:
    start = QML.index("function transitionCaptureEvidence")
    helper = QML[start : QML.index("Connections {", start)]
    assert "overlayCaptureEvidenceStale = stale" in helper
    assert "announceCaptureRecoveryState" in helper
    assert "focusTarget.forceActiveFocus()" in helper
def test_capture_status_paths_use_helper() -> None:
    start = QML.index("target: window.captureMonitorBridge")
    snippet = QML[start : QML.index("target: window.overlayBridge", start)]
    assert snippet.count("transitionCaptureEvidence") == 3
if __name__ == "__main__":
    test_capture_transition_helper_controls_state_announcement_and_focus()
    test_capture_status_paths_use_helper()
    print("2 checks passed")
