"""Contract coverage for safe-mode recovery focus."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_clear_safe_mode_announces_and_focuses_retry() -> None:
    marker = 'objectName: "clearOverlaySafeMode"'
    start = QML.index(marker)
    snippet = QML[start : start + 900]
    assert "resetOverlayRecoveryAttempts()" in snippet
    assert "safe mode cleared" in snippet
    assert "retryOverlay.forceActiveFocus()" in snippet


if __name__ == "__main__":
    test_clear_safe_mode_announces_and_focuses_retry()
    print("1 check passed")
