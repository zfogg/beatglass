"""Contract coverage for stale display-selection recovery failures."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_forget_and_persist_failures_use_stale_transition() -> None:
    forget = QML[QML.index("function forgetUnavailableOverlayDisplay") : QML.index("function persistOverlayDisplaySelection", QML.index("function forgetUnavailableOverlayDisplay"))]
    assert "markOverlayRecoveryStale" in forget
    persist = QML[QML.index("function persistOverlayDisplaySelection") : QML.index("function", QML.index("function persistOverlayDisplaySelection") + 10)]
    assert "markOverlayRecoveryStale" in persist
if __name__ == "__main__":
    test_forget_and_persist_failures_use_stale_transition()
    print("1 check passed")
