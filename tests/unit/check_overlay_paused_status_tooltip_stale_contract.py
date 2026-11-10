"""Contract coverage for paused recovery status tooltip stale boundary."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_paused_status_tooltip_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayRecoveryPausedStatus"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1100]
    assert "overlay evidence is stale until fresh capture" in snippet
if __name__ == "__main__":
    test_paused_status_tooltip_exposes_stale_boundary()
    print("1 check passed")
