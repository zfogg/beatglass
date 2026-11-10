"""Contract coverage for the overlay recovery action stale boundary."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_recovery_action_status_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayRecoveryActionStatus"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1300]
    assert "waiting for a compatible display · live evidence stale" in snippet
    assert "live overlay evidence is stale until you choose or reconnect a display" in snippet


if __name__ == "__main__":
    test_recovery_action_status_exposes_stale_boundary()
    print("1 check passed")
