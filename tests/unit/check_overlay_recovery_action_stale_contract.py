"""Contract coverage for stale overlay recovery action status."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_recovery_action_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayRecoveryActionStatus"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 900]
    assert "evidence stale until fresh capture" in snippet


if __name__ == "__main__":
    test_recovery_action_exposes_stale_boundary()
    print("1 check passed")
