"""Contract coverage for stop-recovery shortcut stale boundary."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_stop_shortcut_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayStopRecoveryShortcutHint"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 900]
    assert "live evidence stale until fresh capture" in snippet
    assert "live evidence is stale until fresh capture arrives" in snippet
if __name__ == "__main__":
    test_stop_shortcut_exposes_stale_boundary()
    print("1 check passed")
