"""Contract coverage for stale visual/settings sync failures."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_visual_and_sync_failures_use_stale_transition() -> None:
    start = QML.index("function persistOverlayDisplaySelection")
    snippet = QML[start : start + 2600]
    assert snippet.count("markOverlayRecoveryStale") >= 3
    assert "overlayDisplaySelectionRequired = true" in snippet
if __name__ == "__main__":
    test_visual_and_sync_failures_use_stale_transition()
    print("1 check passed")
