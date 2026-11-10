"""Contract coverage for native overlay availability transitions."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_overlay_unavailable_marks_stale_and_focuses_retry() -> None:
    start = QML.index("function onAvailableChanged()")
    snippet = QML[start : QML.index("Connections {", start + 20)]
    assert "overlayDisplayRecoveryPending = true" in snippet
    assert "overlayCaptureEvidenceStale = true" in snippet
    assert "Live evidence is stale until fresh capture arrives." in snippet
    assert "retryOverlay.forceActiveFocus()" in snippet
if __name__ == "__main__":
    test_overlay_unavailable_marks_stale_and_focuses_retry()
    print("1 check passed")
