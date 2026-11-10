"""Contract coverage for retrying overlay capture during reacquisition."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_capture_retry_action_is_available_during_reacquisition() -> None:
    marker = 'objectName: "overlayCaptureRecoveryAction"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 1200]
    assert 'status === "Reacquiring"' in snippet
    assert "retry available; reacquiring" in snippet


if __name__ == "__main__":
    test_capture_retry_action_is_available_during_reacquisition()
    print("1 check passed")
