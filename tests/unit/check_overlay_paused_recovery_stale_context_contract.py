"""Contract coverage for the paused overlay recovery accessibility state."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
STATES = (ROOT / "design" / "v1" / "STATES.md").read_text(encoding="utf-8")


def test_paused_status_explains_monitoring_continues_and_resume_is_manual() -> None:
    assert 'objectName: "overlayRecoveryPausedStatus"' in QML
    assert "Recovery paused · monitoring continues" in QML
    assert "start another retry" in QML
    assert 'objectName: "resumeOverlayRecovery"' in QML
    assert "Resume native overlay recovery with a new bounded attempt" in QML


def test_design_state_documents_paused_overlay_recovery() -> None:
    assert "overlay" in STATES.lower()
    assert "resume" in STATES.lower()
    assert "monitoring" in STATES.lower()


if __name__ == "__main__":
    test_paused_status_explains_monitoring_continues_and_resume_is_manual()
    test_design_state_documents_paused_overlay_recovery()
    print("2 checks passed")
