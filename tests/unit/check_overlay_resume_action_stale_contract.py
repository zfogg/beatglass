"""Contract coverage for stale resume recovery action."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_resume_action_exposes_stale_boundary() -> None:
    marker = 'objectName: "resumeOverlayRecovery"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1200]
    assert "live evidence is stale until fresh capture arrives" in snippet
if __name__ == "__main__":
    test_resume_action_exposes_stale_boundary()
    print("1 check passed")
